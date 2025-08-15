/****************************************************************************
Copyright (c) 2013-2016 Chukong Technologies Inc.
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

#include "axmol/ui/UIWidget.h"
#include "axmol/ui/GUIExport.h"

namespace ax
{

/**
 * @addtogroup ui
 * @{
 */

class Label;
struct AX_DLL ResourceData;

namespace ui
{

/**
 * @brief UI TextAtlas widget.
 */
class AX_GUI_DLL TextAtlas : public Widget
{

    DECLARE_CLASS_GUI_INFO

public:
    /**
     * Default constructor.
     *
     * @lua new
     */
    TextAtlas();

    /**
     * Default destructor.
     *
     * @lua NA
     */
    virtual ~TextAtlas();

    /**
     * Create a TexAtlas object.
     *
     * @return An autoreleased TextAtlas object.
     */
    static TextAtlas* create();

    /**
     * Create a LabelAtlas from a char map file.
     *
     * @param stringValue A given string needs to be displayed.
     * @param charMapFile A given char map file name.
     * @param itemWidth The element width.
     * @param itemHeight The element height.
     * @param startCharMap The starting char of the atlas.
     * @return An autoreleased TextAtlas object.
     */
    static TextAtlas* create(std::string_view stringValue,
                             std::string_view charMapFile,
                             int itemWidth,
                             int itemHeight,
                             std::string_view startCharMap);

    /** Initializes the LabelAtlas with a string, a char map file(the atlas), the width and height of each element and
     * the starting char of the atlas.
     *
     * @param stringValue A given string needs to be displayed.
     * @param charMapFile A given char map file name.
     * @param itemWidth The element width.
     * @param itemHeight The element height.
     * @param startCharMap The starting char of the atlas.
     */

    void setProperty(std::string_view stringValue,
                     std::string_view charMapFile,
                     int itemWidth,
                     int itemHeight,
                     std::string_view startCharMap);

    /**Set string value for labelatlas.
     *
     * @param value A given string needs to be displayed.
     */
    void setString(std::string_view value);

    /**Get string value for labelatlas.
     *
     * @return The string value of TextAtlas.
     */
    std::string_view getString() const;

    /**
     * Gets the string length of the label.
     * Note: This length will be larger than the raw string length,
     * if you want to get the raw string length, you should call this->getString().size() instead
     *
     * @return  string length.
     */
    ssize_t getStringLength() const;

    // override "getVirtualRendererSize" method of widget.
    Vec2 getVirtualRendererSize() const override;

    // override "getVirtualRenderer" method of widget.
    Node* getVirtualRenderer() override;

    /**
     * Returns the "class name" of widget.
     */
    std::string getDescription() const override;

    /**
     */
    void adaptRenderers() override;

    ResourceData getRenderFile();

protected:
    void initRenderer() override;
    void onSizeChanged() override;

    void labelAtlasScaleChangedWithSize();
    Widget* createCloneInstance() override;
    void copySpecialProperties(Widget* model) override;

protected:
    Label* _labelAtlasRenderer;
    std::string _stringValue;
    std::string _charMapFileName;
    int _itemWidth;
    int _itemHeight;
    std::string _startCharMap;
    bool _labelAtlasRendererAdaptDirty;
};

}  // namespace ui

// end of ui group
/// @}

}

