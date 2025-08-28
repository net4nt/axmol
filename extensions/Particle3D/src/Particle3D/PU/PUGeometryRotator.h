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

#include "Particle3D/PU/PUAffector.h"
#include "Particle3D/PU/PUDynamicAttribute.h"

namespace ax
{
struct PUParticle3D;
class AX_EX_DLL PUGeometryRotator : public PUAffector
{
public:
    // Constants
    static const bool DEFAULT_USE_OWN;
    static const float DEFAULT_ROTATION_SPEED;
    static const Vec3 DEFAULT_ROTATION_AXIS;

    static PUGeometryRotator* create();

    void updatePUAffector(PUParticle3D* particle, float deltaTime) override;
    /** @copydoc ParticleAffector::_initParticleForEmission */
    void initParticleForEmission(PUParticle3D* particle) override;

    /** Returns the rotation speed. This is the speed controlled by the affector. Besides
        the default rotation speed, it is also possible to use the particles own rotation speed.
    */
    PUDynamicAttribute* getRotationSpeed() const;

    /**
     */
    void setRotationSpeed(PUDynamicAttribute* dynRotationSpeed);

    /** Returns an indication whether the rotation speed is the same for all particles in this
        particle technique, or whether the rotation speed of the particle itself is used.
    */
    bool useOwnRotationSpeed() const;

    /** Set the indication whether rotation speed of the particle itself is used.
     */
    void setUseOwnRotationSpeed(bool _useOwnRotationSpeed);

    /**
     */
    const Vec3& getRotationAxis() const;

    /**
     */
    void setRotationAxis(const Vec3& rotationAxis);

    /**
     */
    void resetRotationAxis();

    void copyAttributesTo(PUAffector* affector) override;

    PUGeometryRotator();
    virtual ~PUGeometryRotator();

protected:
    /** Returns a rotation speed value, depending on the type of dynamic attribute.
     */
    float calculateRotationSpeed(PUParticle3D* particle);

protected:
    float _scaledRotationSpeed;
    bool _useOwnRotationSpeed;
    PUDynamicAttribute* _dynRotationSpeed;
    Quaternion _q;
    Vec3 _rotationAxis;
    bool _rotationAxisSet;

    /** Helper factory
     */
    PUDynamicAttributeHelper _dynamicAttributeHelper;
};
}  // namespace ax
