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

#include "PUBaseCollider.h"
#include "Particle3D/PU/PUSphere.h"
#include "base/Types.h"

namespace ax
{

class AX_EX_DLL PUSphereCollider : public PUBaseCollider
{
public:
    // Constants
    static const float DEFAULT_RADIUS;

    static PUSphereCollider* create();

    void preUpdateAffector(float deltaTime) override;
    void updatePUAffector(PUParticle3D* particle, float deltaTime) override;

    /** Returns the radius of the sphere
     */
    float getRadius() const;

    /** Sets the radius of the sphere
     */
    void setRadius(const float radius);

    /** Returns indication whether the collision is inside or outside of the box
    @remarks
        If value is true, the collision is inside of the box.
    */
    bool isInnerCollision() const;

    /** Set indication whether the collision is inside or outside of the box
    @remarks
        If value is set to true, the collision is inside of the box.
    */
    void setInnerCollision(bool innerCollision);

    /**
     */
    void calculateDirectionAfterCollision(PUParticle3D* particle, Vec3 distance, float distanceLength);

    void copyAttributesTo(PUAffector* affector) override;

    PUSphereCollider();
    virtual ~PUSphereCollider();

protected:
    float _radius;
    PUSphere _sphere;
    Vec3 _predictedPosition;
    bool _innerCollision;
};
}

