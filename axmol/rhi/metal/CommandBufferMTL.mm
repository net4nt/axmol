/****************************************************************************
 Copyright (c) 2018-2019 Xiamen Yaji Software Co., Ltd.
 Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).

 https://axmol.dev/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "axmol/rhi/metal/CommandBufferMTL.h"
#include "axmol/rhi/metal/BufferMTL.h"
#include "axmol/rhi/metal/DriverMTL.h"
#include "axmol/rhi/metal/RenderPipelineMTL.h"
#include "axmol/rhi/metal/TextureMTL.h"
#include "axmol/rhi/metal/UtilsMTL.h"
#include "axmol/rhi/metal/BufferManager.h"
#include "axmol/rhi/metal/DepthStencilStateMTL.h"
#include "axmol/rhi/metal/RenderTargetMTL.h"

namespace ax::rhi::mtl {

namespace
{

#define byte(n) ((n)*8)
#define bit(n) (n)
static uint8_t getBitsPerElementMTL(MTLPixelFormat pixleFormat)
{
    switch (pixleFormat)
    {
    case MTLPixelFormatDepth32Float_Stencil8:
        return byte(8);
    case MTLPixelFormatBGRA8Unorm:
    case MTLPixelFormatRGBA8Unorm:
    case MTLPixelFormatDepth32Float:
        return byte(4);
#if (AX_TARGET_PLATFORM == AX_PLATFORM_MAC)
    case MTLPixelFormatDepth24Unorm_Stencil8:
        return byte(4);
#else
    case MTLPixelFormatABGR4Unorm:
    case MTLPixelFormatBGR5A1Unorm:
    case MTLPixelFormatB5G6R5Unorm:
    case MTLPixelFormatA1BGR5Unorm:
        return byte(2);
#endif
    case MTLPixelFormatA8Unorm:
    case MTLPixelFormatR8Unorm:
        return byte(1);
    default:
        assert(false);
        break;
    }
    return 0;
}

static MTLWinding toMTLWinding(Winding winding)
{
    if (Winding::CLOCK_WISE == winding)
        return MTLWindingClockwise;
    else
        return MTLWindingCounterClockwise;
}

static MTLPrimitiveType toMTLPrimitive(PrimitiveType primitiveType)
{
    MTLPrimitiveType ret = MTLPrimitiveTypeTriangle;
    switch (primitiveType)
    {
    case PrimitiveType::POINT:
        ret = MTLPrimitiveTypePoint;
        break;
    case PrimitiveType::LINE:
        ret = MTLPrimitiveTypeLine;
        break;
    case PrimitiveType::LINE_STRIP:
        ret = MTLPrimitiveTypeLineStrip;
        break;
    case PrimitiveType::TRIANGLE:
        ret = MTLPrimitiveTypeTriangle;
        break;
    case PrimitiveType::TRIANGLE_STRIP:
        ret = MTLPrimitiveTypeTriangleStrip;
        break;
    default:
        break;
    }
    return ret;
}

static MTLIndexType toMTLIndexType(IndexFormat indexFormat)
{
    if (IndexFormat::U_SHORT == indexFormat)
        return MTLIndexTypeUInt16;
    else
        return MTLIndexTypeUInt32;
}

static MTLCullMode toMTLCullMode(CullMode mode)
{
    switch (mode)
    {
    case CullMode::NONE:
        return MTLCullModeNone;
    case CullMode::FRONT:
        return MTLCullModeFront;
    case CullMode::BACK:
        return MTLCullModeBack;
    }
}

static MTLRenderPassDescriptor* toMTLRenderPassDesc(const RenderTarget* rt, const RenderPassDesc& desc)
{
    MTLRenderPassDescriptor* mtlDesc = [MTLRenderPassDescriptor renderPassDescriptor];

    auto rtMTL = static_cast<const RenderTargetImpl*>(rt);
    rtMTL->applyRenderPassAttachments(desc, mtlDesc);
    return mtlDesc;
}

static id<MTLTexture> getMTLTexture(Texture* texture, int index)
{
    return static_cast<TextureImpl*>(texture)->internalHandle(index);
}

static id<MTLSamplerState> getMTLSamplerState(Texture* texture)
{
    return static_cast<TextureImpl*>(texture)->internalSampler();
}

inline int clamp(int value, int min, int max)
{
    return std::min(max, std::max(min, value));
}
}

CommandBufferImpl::CommandBufferImpl(DriverImpl* driver)
    : _mtlCommandQueue(driver->getMTLCommandQueue())
    , _frameBoundarySemaphore(dispatch_semaphore_create(MAX_INFLIGHT_BUFFER))
{}

CommandBufferImpl::~CommandBufferImpl()
{
    // Wait for all frames to finish by submitting and waiting on a dummy command buffer.
    flush();
    id<MTLCommandBuffer> oneOffBuffer = [_mtlCommandQueue commandBuffer];
    [oneOffBuffer commit];
    [oneOffBuffer waitUntilCompleted];

    dispatch_semaphore_signal(_frameBoundarySemaphore);
}

void CommandBufferImpl::setDepthStencilState(DepthStencilState* depthStencilState)
{
    _depthStencilStateImpl = static_cast<DepthStencilStateImpl*>(depthStencilState);
}

void CommandBufferImpl::setRenderPipeline(RenderPipeline* renderPipeline)
{
    _renderPipelineImpl = static_cast<RenderPipelineImpl*>(renderPipeline);
}

bool CommandBufferImpl::beginFrame()
{
    _autoReleasePool = [[NSAutoreleasePool alloc] init];
    dispatch_semaphore_wait(_frameBoundarySemaphore, DISPATCH_TIME_FOREVER);

    _mtlCommandBuffer = [_mtlCommandQueue commandBuffer];
    // [_mtlCommandBuffer enqueue];
    // commit will enqueue automatically
    [_mtlCommandBuffer retain];

    BufferManager::beginFrame();
    return true;
}

void CommandBufferImpl::updateRenderCommandEncoder(const RenderTarget* renderTarget,
                                                  const RenderPassDesc& renderPassDesc)
{
    if (_mtlRenderEncoder != nil && _currentRenderPassDesc == renderPassDesc && _currentRenderTarget == renderTarget &&
        !renderTarget->isDirty())
    {
        return;
    }

    _currentRenderTarget      = renderTarget;
    _currentRenderPassDesc    = renderPassDesc;

    if (_mtlRenderEncoder != nil)
    {
        [_mtlRenderEncoder endEncoding];
        [_mtlRenderEncoder release];
        _mtlRenderEncoder = nil;
    }

    auto mtlDesc  = toMTLRenderPassDesc(renderTarget, renderPassDesc);
    _renderTargetWidth  = (unsigned int)mtlDesc.colorAttachments[0].texture.width;
    _renderTargetHeight = (unsigned int)mtlDesc.colorAttachments[0].texture.height;
    _mtlRenderEncoder   = [_mtlCommandBuffer renderCommandEncoderWithDescriptor:mtlDesc];
    [_mtlRenderEncoder retain];
}

void CommandBufferImpl::beginRenderPass(const RenderTarget* renderTarget, const RenderPassDesc& renderPassDesc)
{
    updateRenderCommandEncoder(renderTarget, renderPassDesc);
    //    [_mtlRenderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
}

void CommandBufferImpl::updateDepthStencilState(const DepthStencilDesc& descriptor)
{
    _depthStencilStateImpl->update(descriptor);
}

void CommandBufferImpl::updatePipelineState(const RenderTarget* rt, const PipelineDesc& descriptor)
{
    _renderPipelineImpl->update(rt, descriptor);
    [_mtlRenderEncoder setRenderPipelineState:_renderPipelineImpl->getMTLRenderPipelineState()];
}

void CommandBufferImpl::setViewport(int x, int y, unsigned int w, unsigned int h)
{
    MTLViewport viewport;
    viewport.originX = x;
    viewport.originY = (int)(_renderTargetHeight - y - h);
    viewport.width   = w;
    viewport.height  = h;
    viewport.znear   = 0;
    viewport.zfar    = 1;
    [_mtlRenderEncoder setViewport:viewport];
}

void CommandBufferImpl::setCullMode(CullMode mode)
{
    [_mtlRenderEncoder setCullMode:toMTLCullMode(mode)];
}

void CommandBufferImpl::setWinding(Winding winding)
{
    [_mtlRenderEncoder setFrontFacingWinding:toMTLWinding(winding)];
}

void CommandBufferImpl::setVertexBuffer(Buffer* buffer)
{
    // Vertex buffer is bound in index DEFAULT_ATTRIBS_BINDING_INDEX.
    [_mtlRenderEncoder setVertexBuffer:static_cast<BufferImpl*>(buffer)->getMTLBuffer() offset:0 atIndex:DriverImpl::DEFAULT_ATTRIBS_BINDING_INDEX];
}

void CommandBufferImpl::setInstanceBuffer(Buffer* buffer) {
    // Vertex instancing transform buffer is bound in index VBO_INSTANCING_BINDING_INDEX.
    // TODO: sync device binding macros to AXSLCC
    [_mtlRenderEncoder setVertexBuffer:static_cast<BufferImpl*>(buffer)->getMTLBuffer() offset:0 atIndex:DriverImpl::VBO_INSTANCING_BINDING_INDEX];
}

void CommandBufferImpl::setProgramState(ProgramState* programState)
{
    AX_SAFE_RETAIN(programState);
    AX_SAFE_RELEASE(_programState);
    _programState = programState;
}

void CommandBufferImpl::setIndexBuffer(Buffer* buffer)
{
    assert(buffer != nullptr);
    if (!buffer)
        return;

    _mtlIndexBuffer = static_cast<BufferImpl*>(buffer)->getMTLBuffer();
    [_mtlIndexBuffer retain];
}

void CommandBufferImpl::drawArrays(PrimitiveType primitiveType, std::size_t start, std::size_t count, bool wireframe /* unused */)
{
    prepareDrawing();
    [_mtlRenderEncoder drawPrimitives:toMTLPrimitive(primitiveType) vertexStart:start vertexCount:count];
}

void CommandBufferImpl::drawArraysInstanced(PrimitiveType primitiveType, std::size_t start, std::size_t count, int instanceCount, bool wireframe /* unused */)
{
    prepareDrawing();
    [_mtlRenderEncoder drawPrimitives:toMTLPrimitive(primitiveType) vertexStart:start vertexCount:count instanceCount:instanceCount];
}

void CommandBufferImpl::drawElements(PrimitiveType primitiveType,
                                    IndexFormat indexType,
                                    std::size_t count,
                                    std::size_t offset,
									bool /* wireframe */)
{
    prepareDrawing();
    [_mtlRenderEncoder drawIndexedPrimitives:toMTLPrimitive(primitiveType)
                                indexCount:count
                                indexType:toMTLIndexType(indexType)
                                indexBuffer:_mtlIndexBuffer
                                indexBufferOffset:offset];
}

void CommandBufferImpl::drawElementsInstanced(PrimitiveType primitiveType,
                           IndexFormat indexType,
                           std::size_t count,
                           std::size_t offset,
                           int instanceCount,
                           bool /* wireframe */)
{
    prepareDrawing();
    [_mtlRenderEncoder drawIndexedPrimitives:toMTLPrimitive(primitiveType)
                                indexCount:count
                                indexType:toMTLIndexType(indexType)
                                indexBuffer:_mtlIndexBuffer
                                indexBufferOffset:offset
                                instanceCount:instanceCount];
}

void CommandBufferImpl::endRenderPass()
{
    afterDraw();
}

void CommandBufferImpl::readPixels(RenderTarget* rt, std::function<void(const PixelBufferDesc&)> callback)
{
    auto rtMTL = static_cast<RenderTargetImpl*>(rt);

    // we only read form color attachment 0
    // if it's nullptr, will regard as screen to perform capture
    auto texture = rtMTL->_color[0].texture;
    AX_SAFE_RETAIN(texture);
    _captureCallbacks.emplace_back(texture, std::move(callback));
}

void CommandBufferImpl::endFrame()
{
    [_mtlRenderEncoder endEncoding];
    [_mtlRenderEncoder release];
    _mtlRenderEncoder = nil;

    auto currentDrawable = DriverImpl::getCurrentDrawable();
    [_mtlCommandBuffer presentDrawable:currentDrawable];
    _drawableTexture = currentDrawable.texture;
    [_mtlCommandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer) {
      // GPU work is complete
      // Signal the semaphore to start the CPU work
      dispatch_semaphore_signal(_frameBoundarySemaphore);
    }];

    flush();

    DriverImpl::resetCurrentDrawable();
    [_autoReleasePool drain];
}

void CommandBufferImpl::endEncoding()
{
    if (_mtlRenderEncoder) {
        [_mtlRenderEncoder endEncoding];
        [_mtlRenderEncoder release];
    }
    _mtlRenderEncoder = nil;
}

void CommandBufferImpl::flush()
{
    if (_mtlCommandBuffer)
    {
        assert(_mtlCommandBuffer.status != MTLCommandBufferStatusCommitted);
        [_mtlCommandBuffer commit];

        flushCaptureCommands();

        [_mtlCommandBuffer release];
        _mtlCommandBuffer = nil;
    }
}

void CommandBufferImpl::flushCaptureCommands()
{
    if (!_captureCallbacks.empty())
    {
        // !!!important, if have capture request, must wait pending commandBuffer finish at this frame,
        // because readPixels require sync operation to get screen pixels properly without data race issue,
        // otherwise, will lead dead-lock
        // !!!Notes, MTL is mutli-threading, all GPU handler is dispatch at GPU threads
        [_mtlCommandBuffer waitUntilCompleted];

        PixelBufferDesc screenPixelData;
        for (auto& cb : _captureCallbacks)
        {
            if (cb.first == nil)
            {  // screen capture
                if (!screenPixelData)
                {
                    CommandBufferImpl::readPixels(_drawableTexture, 0, 0, [_drawableTexture width],
                                                 [_drawableTexture height], screenPixelData);
                    // screen framebuffer copied, restore screen framebuffer only to true
                    rhi::DriverBase::getInstance()->setFrameBufferOnly(true);
                }
                cb.second(screenPixelData);
            }
            else
            {
                PixelBufferDesc pixelData;
                auto texture = cb.first;
                assert(texture != nullptr);
                CommandBufferImpl::readPixels(texture, 0, 0, texture->getWidth(), texture->getHeight(), pixelData);
                AX_SAFE_RELEASE(texture);
                cb.second(pixelData);
            }
        }
        _captureCallbacks.clear();
    }
}

void CommandBufferImpl::afterDraw()
{
    if (_mtlIndexBuffer)
    {
        [_mtlIndexBuffer release];
        _mtlIndexBuffer = nullptr;
    }

    AX_SAFE_RELEASE_NULL(_programState);
}

void CommandBufferImpl::prepareDrawing() const
{
    setUniformBuffer();
    setTextures();

    auto mtlDepthStencilState = _depthStencilStateImpl->getMTLDepthStencilState();
    if (mtlDepthStencilState)
    {
        [_mtlRenderEncoder setDepthStencilState:mtlDepthStencilState];
        [_mtlRenderEncoder setStencilFrontReferenceValue:_stencilReferenceValue
                                      backReferenceValue:_stencilReferenceValue];
    }
}

void CommandBufferImpl::setTextures() const
{
    if (_programState)
    {
        doSetTextures(true);
        doSetTextures(false);
    }
}

void CommandBufferImpl::doSetTextures(bool isVertex) const
{
    const auto& bindTextureInfos =
        (isVertex) ? _programState->getVertexTextureInfos() : _programState->getFragmentTextureInfos();

    for (const auto& iter : bindTextureInfos)
    {
        /* About mutli textures support
         *  a. TODO: sampler2DArray, not implemented in Metal Renderer currently
         *  b. texture slot, one BackendTexture, multi GPU texture handlers, used by etc1, restrict: textures must have
         * same size c. Bind multi BackendTexture to 1 Shader Program, see the ShaderTest d. iter.second.slots not used
         * for Metal Renderer
         */
        auto location  = iter.first;
        auto& textures = iter.second.textures;
        auto& indexs   = iter.second.indexs;

        auto texture = textures[0];
        auto index   = indexs[0];

        if (isVertex)
        {
            [_mtlRenderEncoder setVertexTexture:getMTLTexture(texture, index) atIndex:location];
            [_mtlRenderEncoder setVertexSamplerState:getMTLSamplerState(texture) atIndex:location];
        }
        else
        {
            [_mtlRenderEncoder setFragmentTexture:getMTLTexture(texture, index) atIndex:location];
            [_mtlRenderEncoder setFragmentSamplerState:getMTLSamplerState(texture) atIndex:location];
        }
    }
}

void CommandBufferImpl::setUniformBuffer() const
{
    if (_programState)
    {
        auto& callbackUniforms = _programState->getCallbackUniforms();
        for (auto& cb : callbackUniforms)
            cb.second(_programState, cb.first);

        // Uniform buffer: glsl-optimizer is bound to index 1, axslcc: bound to 0
        constexpr int bindingIndex = DriverImpl::VBO_BINDING_INDEX_START;
        std::size_t bufferSize = 0;
        auto vertexBuffer     = _programState->getVertexUniformBuffer(bufferSize);
        if (bufferSize)
        {
            [_mtlRenderEncoder setVertexBytes:vertexBuffer length:bufferSize atIndex:bindingIndex];
        }

        auto fragmentBuffer = _programState->getFragmentUniformBuffer(bufferSize);
        if (bufferSize)
        {
            [_mtlRenderEncoder setFragmentBytes:fragmentBuffer length:bufferSize atIndex:bindingIndex];
        }
    }
}

void CommandBufferImpl::setScissorRect(bool isEnabled, float x, float y, float width, float height)
{
    MTLScissorRect scissorRect;
    if (isEnabled)
    {
        y                  = _renderTargetHeight - height - y;
        int minX           = clamp((int)x, 0, (int)_renderTargetWidth);
        int minY           = clamp((int)y, 0, (int)_renderTargetHeight);
        int maxX           = clamp((int)(x + width), 0, (int)_renderTargetWidth);
        int maxY           = clamp((int)(y + height), 0, (int)_renderTargetHeight);
        scissorRect.x      = minX;
        scissorRect.y      = minY;
        scissorRect.width  = maxX - minX;
        scissorRect.height = maxY - minY;
        if (scissorRect.width == 0 || scissorRect.height == 0)
        {
            scissorRect.width  = 0;
            scissorRect.height = 0;
        }
    }
    else
    {
        scissorRect.x      = 0;
        scissorRect.y      = 0;
        scissorRect.width  = _renderTargetWidth;
        scissorRect.height = _renderTargetHeight;
    }
    [_mtlRenderEncoder setScissorRect:scissorRect];
}

void CommandBufferImpl::readPixels(Texture* texture,
                                  std::size_t origX,
                                  std::size_t origY,
                                  std::size_t rectWidth,
                                  std::size_t rectHeight,
                                  PixelBufferDesc& pbd)
{
    CommandBufferImpl::readPixels(static_cast<TextureImpl*>(texture)->internalHandle(), origX, origY, rectWidth,
                                 rectHeight, pbd);
}

void CommandBufferImpl::readPixels(id<MTLTexture> texture,
                                  std::size_t origX,
                                  std::size_t origY,
                                  std::size_t rectWidth,
                                  std::size_t rectHeight,
                                  PixelBufferDesc& pbd)
{
    NSUInteger texWidth   = texture.width;
    NSUInteger texHeight  = texture.height;
    MTLRegion region      = MTLRegionMake2D(0, 0, texWidth, texHeight);
    MTLRegion imageRegion = MTLRegionMake2D(origX, origY, rectWidth, rectHeight);

    MTLTextureDescriptor* textureDesc =
        [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:[texture pixelFormat]
                                                           width:texWidth
                                                          height:texHeight
                                                       mipmapped:NO];
    id<MTLDevice> device             = static_cast<DriverImpl*>(DriverBase::getInstance())->getMTLDevice();
    id<MTLTexture> readPixelsTexture = [device newTextureWithDescriptor:textureDesc];

    id<MTLCommandQueue> commandQueue = static_cast<DriverImpl*>(DriverBase::getInstance())->getMTLCommandQueue();
    auto commandBuffer               = [commandQueue commandBuffer];
    // [commandBuffer enqueue];

    id<MTLBlitCommandEncoder> blitCommandEncoder = [commandBuffer blitCommandEncoder];
    [blitCommandEncoder copyFromTexture:texture
                            sourceSlice:0
                            sourceLevel:0
                           sourceOrigin:region.origin
                             sourceSize:region.size
                              toTexture:readPixelsTexture
                       destinationSlice:0
                       destinationLevel:0
                      destinationOrigin:region.origin];

#if (AX_TARGET_PLATFORM == AX_PLATFORM_MAC)
    [blitCommandEncoder synchronizeResource:readPixelsTexture];
#endif
    [blitCommandEncoder endEncoding];

    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBufferMTL) {
      auto bytePerRow = rectWidth * getBitsPerElementMTL(texture.pixelFormat) / 8;
      auto texelData  = pbd._data.resize(bytePerRow * rectHeight);
      if (texelData != nullptr)
      {
          [readPixelsTexture getBytes:texelData bytesPerRow:bytePerRow fromRegion:imageRegion mipmapLevel:0];
          UtilsMTL::swizzleImage(texelData, rectWidth, rectHeight, readPixelsTexture.pixelFormat);
          pbd._width  = static_cast<int>(rectWidth);
          pbd._height = static_cast<int>(rectHeight);
      }
      [readPixelsTexture release];
    }];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

}
