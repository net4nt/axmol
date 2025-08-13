/*
 * Copyright (c) 2012 cocos2d-x.org
 * https://axmol.dev/
 *
 * Copyright 2012 Stewart Hamilton-Arrandale.
 * http://creativewax.co.uk
 *
 * Modified by Yannick Loriot.
 * http://yannickloriot.com
 *
 * Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Converted to c++ / cocos2d-x by Angus C
 */

#pragma once

#include "Control.h"
#include "ControlUtils.h"
#include "ControlHuePicker.h"
#include "ControlSaturationBrightnessPicker.h"
#include "extensions/ExtensionExport.h"

NS_AX_EXT_BEGIN

/**
 * @addtogroup GUI
 * @{
 * @addtogroup control_extension
 * @{
 */

class AX_EX_DLL ControlColourPicker : public Control
{
public:
    static ControlColourPicker* create();
    /**
     * @lua new
     */
    ControlColourPicker();
    /**
     * @lua NA
     */
    virtual ~ControlColourPicker();

    bool init() override;

    void setColor(const Color32& colorValue) override;
    void setEnabled(bool bEnabled) override;

    // virtual ~ControlColourPicker();
    void hueSliderValueChanged(Object* sender, Control::EventType controlEvent);
    void colourSliderValueChanged(Object* sender, Control::EventType controlEvent);

protected:
    void updateControlPicker();
    void updateHueAndControlPicker();
    bool onTouchBegan(Touch* touch, Event* pEvent) override;

    HSV _hsv;
    AX_SYNTHESIZE_RETAIN(ControlSaturationBrightnessPicker*, _colourPicker, colourPicker)
    AX_SYNTHESIZE_RETAIN(ControlHuePicker*, _huePicker, HuePicker)
    AX_SYNTHESIZE_RETAIN(Sprite*, _background, Background)
};

// end of GUI group
/// @}
/// @}

NS_AX_EXT_END
