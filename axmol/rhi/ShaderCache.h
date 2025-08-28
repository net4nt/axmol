/****************************************************************************
 Copyright (c) 2018 Xiamen Yaji Software Co., Ltd.
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
#pragma once

#include "axmol/rhi/RHITypes.h"
#include "axmol/base/Object.h"
#include "axmol/platform/PlatformMacros.h"
#include "axmol/rhi/ShaderModule.h"

#include <string>
#include <unordered_map>

namespace ax::rhi
{
/**
 * @addtogroup _rhi
 * @{
 */

/**
 * Create and reuse shader module.
 */
class AX_DLL ShaderCache
{
public:
    static ShaderCache* getInstance();
    static void destroyInstance();

    ~ShaderCache();

    /** purges the cache. It releases the retained instance. */
    void purge();

    /**
     * Create a vertex shader module and add it to cache.
     * If it is created before, then just return the cached shader module.
     * @param shaderSource The source code of the shader.
     */
    rhi::ShaderModule* acquireVertexShaderModule(std::string_view shaderSource);

    /**
     * Create a fragment shader module.
     * If it is created before, then just return the cached shader module.
     * @param shaderSource The source code of the shader.
     */
    rhi::ShaderModule* acquireFragmentShaderModule(std::string_view shaderSource);

    /**
     * Remove all unused shaders.
     */
    void removeUnusedShader();

protected:
    /**
     * New a shaderModule.
     * If it was created before, then just return the cached shader module.
     * Otherwise add it to cache and return the object.
     * @param stage Specifies whether is vertex shader or fragment shader.
     * @param source Specifies shader source.
     * @return A ShaderModule object.
     */
    rhi::ShaderModule* acquireShaderModule(rhi::ShaderStage stage, std::string_view shaderSource);

    std::unordered_map<uint64_t, rhi::ShaderModule*> _cachedShaders;
};

// end of _rhi group
/// @}
}  // namespace ax::rhi
