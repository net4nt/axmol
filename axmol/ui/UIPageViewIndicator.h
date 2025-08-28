/****************************************************************************
Copyright (c) 2015 Neo Kim (neo.kim@neofect.com)
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
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

#include "axmol/ui/UIPageView.h"
#include "axmol/2d/Sprite.h"

namespace ax
{
/**
 * @addtogroup ui
 * @{
 */

namespace ui
{

class PageViewIndicator : public ProtectedNode
{

public:
    /**
     * Create a page view indicator with its parent page view.
     * @return A page view indicator instance.
     */
    static PageViewIndicator* create();

    PageViewIndicator();
    virtual ~PageViewIndicator();

    void setDirection(PageView::Direction direction);
    void reset(ssize_t numberOfTotalPages);
    void indicate(ssize_t index);
    void clear();
    void setSpaceBetweenIndexNodes(float spaceBetweenIndexNodes);
    float getSpaceBetweenIndexNodes() const { return _spaceBetweenIndexNodes; }
    void setSelectedIndexColor(const Color32& color) { _currentIndexNode->setColor(color); }
    const Color32& getSelectedIndexColor() const { return _currentIndexNode->getColor(); }
    void setIndexNodesColor(const Color32& indexNodesColor);
    const Color32& getIndexNodesColor() const { return _indexNodesColor; }
    void setIndexNodesScale(float indexNodesScale);
    float getIndexNodesScale() const { return _indexNodesScale; }
    void setSelectedIndexOpacity(uint8_t opacity) { _currentIndexNode->setOpacity(opacity); }
    uint8_t getSelectedIndexOpacity() const { return _currentIndexNode->getOpacity(); }
    void setIndexNodesOpacity(uint8_t opacity);
    uint8_t getIndexNodesOpacity() const { return _indexNodesColor.a; }

    /**
     * Sets texture for index nodes.
     *
     * @param fileName   File name of texture.
     * @param resType    @see TextureResType .
     */
    void setIndexNodesTexture(std::string_view texName, Widget::TextureResType texType = Widget::TextureResType::LOCAL);

protected:
    bool init() override;
    void increaseNumberOfPages();
    void decreaseNumberOfPages();
    void rearrange();

    PageView::Direction _direction;
    Vector<Sprite*> _indexNodes;
    Sprite* _currentIndexNode;
    Sprite* _currentOverlappingIndexNode;
    float _spaceBetweenIndexNodes;
    float _indexNodesScale;
    Color32 _indexNodesColor;

    bool _useDefaultTexture;
    std::string _indexNodesTextureFile;
    Widget::TextureResType _indexNodesTexType;
};

}  // namespace ui
// end of ui group
/// @}
}  // namespace ax
