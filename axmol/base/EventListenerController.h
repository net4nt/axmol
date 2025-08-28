/****************************************************************************
 Copyright (c) 2014 cocos2d-x.org
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

#include "axmol/platform/PlatformMacros.h"
#include "axmol/base/EventListener.h"

/**
 * @addtogroup base
 * @{
 */

namespace ax
{

class Event;
class Controller;

/** @class EventListenerController
 * @param Controller event listener.
 */
class AX_DLL EventListenerController : public EventListener
{
public:
    static const std::string LISTENER_ID;

    /** Create a controller event listener.
     *
     * @return An autoreleased EventListenerController object.
     */
    static EventListenerController* create();

    /// Overrides
    bool checkAvailable() override;
    EventListenerController* clone() override;

    std::function<void(Controller*, Event*)> onConnected;
    std::function<void(Controller*, Event*)> onDisconnected;

    std::function<void(Controller*, int, Event*)> onKeyDown;
    std::function<void(Controller*, int, Event*)> onKeyUp;
    std::function<void(Controller*, int, Event*)> onKeyRepeat;

    std::function<void(Controller*, int, Event*)> onAxisEvent;

protected:
    bool init();
};

}  // namespace ax

// end of base group
/// @}
