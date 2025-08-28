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
#include "axmol/math/Math.h"
#include "Particle3D/PU/PUObserver.h"
#include <vector>
#include <string>

namespace ax
{

struct PUParticle3D;
class PUParticleSystem3D;
class AX_EX_DLL PUOnPositionObserver : public PUObserver
{
public:
    // Constants
    static const Vec3 DEFAULT_POSITION_THRESHOLD;

    static PUOnPositionObserver* create();

    /**
     */
    bool observe(PUParticle3D* particle, float timeElapsed) override;

    /**
     */
    void setPositionXThreshold(float threshold)
    {
        _positionXThreshold    = threshold;
        _positionXThresholdSet = true;
    };
    void setPositionYThreshold(float threshold)
    {
        _positionYThreshold    = threshold;
        _positionYThresholdSet = true;
    };
    void setPositionZThreshold(float threshold)
    {
        _positionZThreshold    = threshold;
        _positionZThresholdSet = true;
    };

    /**
     */
    float getPositionXThreshold() const { return _positionXThreshold; };
    float getPositionYThreshold() const { return _positionYThreshold; };
    float getPositionZThreshold() const { return _positionZThreshold; };

    /**
     */
    bool isPositionXThresholdSet() const { return _positionXThresholdSet; };
    bool isPositionYThresholdSet() const { return _positionYThresholdSet; };
    bool isPositionZThresholdSet() const { return _positionZThresholdSet; };

    /**
     */
    void resetPositionXThreshold() { _positionXThresholdSet = false; };
    void resetPositionYThreshold() { _positionYThresholdSet = false; };
    void resetPositionZThreshold() { _positionZThresholdSet = false; };

    /**
     */
    void setComparePositionX(PUComparisionOperator op) { _comparePositionX = op; };
    void setComparePositionY(PUComparisionOperator op) { _comparePositionY = op; };
    void setComparePositionZ(PUComparisionOperator op) { _comparePositionZ = op; };

    /**
     */
    PUComparisionOperator getComparePositionX() const { return _comparePositionX; };
    PUComparisionOperator getComparePositionY() const { return _comparePositionY; };
    PUComparisionOperator getComparePositionZ() const { return _comparePositionZ; };

    void copyAttributesTo(PUObserver* observer) override;

    PUOnPositionObserver();
    virtual ~PUOnPositionObserver() {};

protected:
    float _positionXThreshold;
    float _positionYThreshold;
    float _positionZThreshold;
    bool _positionXThresholdSet;
    bool _positionYThresholdSet;
    bool _positionZThresholdSet;
    PUComparisionOperator _comparePositionX;
    PUComparisionOperator _comparePositionY;
    PUComparisionOperator _comparePositionZ;
};

}  // namespace ax
