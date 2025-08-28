/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2016 Chukong Technologies Inc.
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

#include "axmol/2d/Node.h"
#include "axmol/base/Vector.h"

namespace ax
{

/**
 * @addtogroup _2d
 * @{
 */

/** @class ParallaxNode
 * @brief ParallaxNode: A node that simulates a parallax scroller

The children will be moved faster / slower than the parent according the parallax ratio.

*/
class AX_DLL ParallaxNode : public Node
{
public:
    /** Create a Parallax node.
     *
     * @return An autoreleased ParallaxNode object.
     */
    static ParallaxNode* create();

    // prevents compiler warning: "Included function hides overloaded virtual functions"
    using Node::addChild;

    /** Adds a child to the container with a local z-order, parallax ratio and position offset.
     *
     * @param child A child node.
     * @param z Z order for drawing priority.
     * @param parallaxRatio A given parallax ratio.
     * @param positionOffset A given position offset.
     */
    void addChild(Node* child, int z, const Vec2& parallaxRatio, const Vec2& positionOffset);

    /** Sets an array of layers for the Parallax node.
     *
     * @param parallaxArray An array of layers for the Parallax node.
     */
    void setParallaxArray(Vector<Object*> parallaxArray) { _parallaxArray = std::move(parallaxArray); }
    /** Returns the array of layers of the Parallax node.
     *
     * @return An array of layers for the Parallax node.
     */
    Vector<Object*>& getParallaxArray() { return _parallaxArray; }
    const Vector<Object*>& getParallaxArray() const { return _parallaxArray; }

    //
    // Overrides
    //
    void addChild(Node* child, int zOrder, int tag) override;
    void addChild(Node* child, int zOrder, std::string_view name) override;
    void removeChild(Node* child, bool cleanup) override;
    void removeAllChildrenWithCleanup(bool cleanup) override;
    void visit(Renderer* renderer, const Mat4& parentTransform, uint32_t parentFlags) override;

    /** Adds a child to the container with a z-order, a parallax ratio and a position offset
     It returns self, so you can chain several addChilds.
     @since v0.8
     */
    ParallaxNode();
    /**
     * @lua NA
     */
    virtual ~ParallaxNode();

protected:
    Vec2 absolutePosition();

    Vec2 _lastPosition;
    Vector<Object*> _parallaxArray;

private:
    AX_DISALLOW_COPY_AND_ASSIGN(ParallaxNode);
};

// end of _2d group
/// @}

}  // namespace ax
