/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2015 hanxi
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

#include "platform/PlatformConfig.h"

#if (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX)

#    include "ui/UIEditBox/UIEditBoxImpl-common.h"

namespace ax
{

class Label;

namespace ui
{

class EditBox;

class EditBoxImplLinux : public EditBoxImplCommon
{
public:
    /**
     */
    EditBoxImplLinux(EditBox* pEditText);
    /**
     * @lua NA
     */
    virtual ~EditBoxImplLinux();

    bool isEditing() override;
    void createNativeControl(const Rect& frame) override{};
    void setNativeFont(const char* pFontName, int fontSize) override{};
    void setNativeFontColor(const Color32& color) override{};
    void setNativePlaceholderFont(const char* pFontName, int fontSize) override{};
    void setNativePlaceholderFontColor(const Color32& color) override{};
    void setNativeInputMode(EditBox::InputMode inputMode) override{};
    void setNativeInputFlag(EditBox::InputFlag inputFlag) override{};
    void setNativeReturnType(EditBox::KeyboardReturnType returnType) override{};
    virtual void setNativeTextHorizontalAlignment(ax::TextHAlignment alignment){};
    void setNativeText(const char* pText) override{};
    void setNativePlaceHolder(const char* pText) override{};
    void setNativeVisible(bool visible) override{};
    void updateNativeFrame(const Rect& rect) override{};
    const char* getNativeDefaultFontName() override{ return ""; };
    void nativeOpenKeyboard() override;
    void nativeCloseKeyboard() override{};
    void setNativeMaxLength(int maxLength) override{};

private:
    void doAnimationWhenKeyboardMove(float duration, float distance) override {}
};

}  // namespace ui

}

#endif /* #if (AX_TARGET_PLATFORM == AX_PLATFORM_LINUX) */

