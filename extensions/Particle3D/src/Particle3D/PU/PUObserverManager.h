/****************************************************************************
 Copyright (C) 2013 Henry van Merode. All rights reserved.
 Copyright (c) 2015-2016 Chukong Technologies Inc.
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

#include "axmol/base/Object.h"
#include "Particle3D/PU/PUScriptTranslator.h"
#include "Particle3D/PU/PUObserver.h"
#include "Particle3D/PU/PUOnClearObserverTranslator.h"
#include "Particle3D/PU/PUOnCollisionObserverTranslator.h"
#include "Particle3D/PU/PUOnCountObserverTranslator.h"
#include "Particle3D/PU/PUOnEmissionObserverTranslator.h"
#include "Particle3D/PU/PUOnEventFlagObserverTranslator.h"
#include "Particle3D/PU/PUOnExpireObserverTranslator.h"
#include "Particle3D/PU/PUOnPositionObserverTranslator.h"
#include "Particle3D/PU/PUOnQuotaObserverTranslator.h"
#include "Particle3D/PU/PUOnRandomObserverTranslator.h"
#include "Particle3D/PU/PUOnTimeObserverTranslator.h"
#include "Particle3D/PU/PUOnVelocityObserverTranslator.h"

namespace ax
{
class PUObserverManager
{
public:
    static PUObserverManager* Instance();

    /**
     */
    PUScriptTranslator* getTranslator(std::string_view type);
    PUObserver* createObserver(std::string_view type);

    PUObserverManager();
    ~PUObserverManager();

protected:
    PUOnClearObserverTranslator _onClearObserverTranslator;
    PUOnCollisionObserverTranslator _onCollisionObserverTranslator;
    PUOnCountObserverTranslator _onCountObserverTranslator;
    PUOnEmissionObserverTranslator _onEmissionObserverTranslator;
    PUOnEventFlagObserverTranslator _onEventFlagObserverTranslator;
    PUOnExpireObserverTranslator _onExpireObserverTranslator;
    PUOnPositionObserverTranslator _onPositionObserverTranslator;
    PUOnQuotaObserverTranslator _onQuotaObserverTranslator;
    PUOnRandomObserverTranslator _onRandomObserverTranslator;
    PUOnTimeObserverTranslator _onTimeObserverTranslator;
    PUOnVelocityObserverTranslator _onVelocityObserverTranslator;
};

}  // namespace ax
