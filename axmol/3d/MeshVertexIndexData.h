/****************************************************************************
 Copyright (c) 2014-2016 Chukong Technologies Inc.
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

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

#include <string>
#include <vector>

#include "axmol/3d/Bundle3DData.h"
#include "axmol/3d/AABB.h"
#include "axmol/3d/shaderinfos.h"

#include "axmol/base/Object.h"
#include "axmol/base/Vector.h"
#include "axmol/math/Math.h"
#include "axmol/renderer/MeshCommand.h"

namespace ax
{

/**
 * @addtogroup _3d
 * @{
 */

class MeshVertexData;

/**
 * the MeshIndexData class.
 * @brief the MeshIndexData contain all of the indices data which mesh need.
 * @lua NA
 */
class AX_DLL MeshIndexData : public Object
{
public:
    /** create  */
    static MeshIndexData* create(std::string_view id,
                                 MeshVertexData* vertexData,
                                 rhi::Buffer* indexbuffer,
                                 const AABB& aabb);

    /**get index buffer*/
    rhi::Buffer* getIndexBuffer() const { return _indexBuffer; }

    /**get vertex buffer*/
    rhi::Buffer* getVertexBuffer() const;

    /**get vertex data*/
    const MeshVertexData* getMeshVertexData() const { return _vertexData; }

    /** aabb getter and setter */
    void setAABB(const AABB& aabb) { _aabb = aabb; }
    const AABB& getAABB() const { return _aabb; }

    /** id setter and getter */
    void setId(std::string_view id) { _id = id; }
    std::string_view getId() const { return _id; }

    /**primitive type setter & getter*/
    MeshCommand::PrimitiveType getPrimitiveType() const { return _primitiveType; }
    void setPrimitiveType(MeshCommand::PrimitiveType primitive) { _primitiveType = primitive; }

    void setIndexData(const MeshData::IndexArray& indexdata);

    MeshIndexData();
    virtual ~MeshIndexData();

protected:
    rhi::Buffer* _indexBuffer = nullptr;  // index buffer
    MeshVertexData* _vertexData   = nullptr;  // vertex buffer, weak ref
    AABB _aabb;                               // original aabb of the submesh
    std::string _id;                          // id
    MeshCommand::PrimitiveType _primitiveType = MeshCommand::PrimitiveType::TRIANGLE;
    MeshData::IndexArray _indexData;

    friend class MeshVertexData;
    friend class MeshRenderer;
#if AX_ENABLE_CONTEXT_LOSS_RECOVERY
    EventListenerCustom* _backToForegroundListener = nullptr;
#endif
};

/**
 * the MeshVertexData class.
 * @brief the MeshVertexData contain all of the vertices data which mesh need.
 */
class AX_DLL MeshVertexData : public Object
{
    friend class MeshRenderer;
    friend class Mesh;

public:
    /**create*/
    static MeshVertexData* create(const MeshData& meshdata, CustomCommand::IndexFormat format = CustomCommand::IndexFormat::U_SHORT);

    /** get vertexbuffer */
    rhi::Buffer* getVertexBuffer() const { return _vertexBuffer; }

    /** get attributes count */
    ssize_t getMeshVertexAttribCount() const { return _attribs.size(); }

    /** get attribute by index */
    const MeshVertexAttrib& getMeshVertexAttrib(ssize_t index) const { return _attribs[index]; }

    /** get index data count */
    ssize_t getMeshIndexDataCount() const { return _indices.size(); }
    /** get index data by index */
    MeshIndexData* getMeshIndexDataByIndex(int index) const { return _indices.at(index); }
    /** get index data by id */
    MeshIndexData* getMeshIndexDataById(std::string_view id) const;

    ssize_t getSizePerVertex() const { return _sizePerVertex; }

    /**has vertex attribute?*/
    // TODO: will be removed!
    bool hasVertexAttrib(shaderinfos::VertexKey attrib) const;

    void setVertexData(const std::vector<float>& vertexData);

    MeshVertexData();
    virtual ~MeshVertexData();

protected:
    rhi::Buffer* _vertexBuffer = nullptr;  // vertex buffer
    ssize_t _sizePerVertex         = -1;
    Vector<MeshIndexData*> _indices;           // index data
    axstd::pod_vector<MeshVertexAttrib> _attribs;    // vertex attributes

    int _vertexCount = 0;  // vertex count
    std::vector<float> _vertexData;
#if AX_ENABLE_CONTEXT_LOSS_RECOVERY
    EventListenerCustom* _backToForegroundListener = nullptr;
#endif
};

// end of 3d group
/// @}

}
