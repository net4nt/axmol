/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
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

#include <vector>
#include <string>
#include "axmol/base/Object.h"

/**
 * @addtogroup base
 * @{
 */
namespace ax
{

/**
 * A pool for managing autorelease objects.
 */
class AX_DLL AutoreleasePool
{
public:
    /**
     * @warning Don't create an autorelease pool in heap, create it in stack.
     * @lua NA
     */
    AutoreleasePool();

    /**
     * Create an autorelease pool with specific name. This name is useful for debugging.
     * @warning Don't create an autorelease pool in heap, create it in stack.
     * @lua NA
     *
     * @param name The name of created autorelease pool.
     */
    AutoreleasePool(std::string_view name);

    /**
     * @lua NA
     */
    ~AutoreleasePool();

    /**
     * Add a given object to this autorelease pool.
     *
     * The same object may be added several times to an autorelease pool. When the
     * pool is destructed, the object's `Object::release()` method will be called
     * the same times as it was added.
     *
     * @param object    The object to be added into the autorelease pool.
     * @lua NA
     */
    void addObject(Object* object);

    /**
     * Clear the autorelease pool.
     *
     * It will invoke each element's `release()` function.
     *
     * @lua NA
     */
    void clear();

#if defined(_AX_DEBUG) && (_AX_DEBUG > 0)
    /**
     * Whether the autorelease pool is doing `clear` operation.
     *
     * @return True if autorelease pool is clearing, false if not.
     *
     * @lua NA
     */
    bool isClearing() const { return _isClearing; };
#endif

    /**
     * Checks whether the autorelease pool contains the specified object.
     *
     * @param object The object to be checked.
     * @return True if the autorelease pool contains the object, false if not
     * @lua NA
     */
    bool contains(Object* object) const;

    /**
     * Dump the objects that are put into the autorelease pool. It is used for debugging.
     *
     * The result will look like:
     * Object pointer address     object id     reference count
     *
     * @lua NA
     */
    void dump();

private:
    /**
     * The underlying array of object managed by the pool.
     *
     * Although Array retains the object once when an object is added, proper
     * Object::release() is called outside the array to make sure that the pool
     * does not affect the managed object's reference count. So an object can
     * be destructed properly by calling Object::release() even if the object
     * is in the pool.
     */
    std::vector<Object*> _managedObjectArray;
    std::string _name;

#if defined(_AX_DEBUG) && (_AX_DEBUG > 0)
    /**
     *  The flag for checking whether the pool is doing `clear` operation.
     */
    bool _isClearing;
#endif
};

// end of base group
/** @} */

/**
 * @cond
 */
class AX_DLL PoolManager
{
public:
    static PoolManager* getInstance();
    static void destroyInstance();

    /**
     * Get current auto release pool, there is at least one auto release pool that created by engine.
     * You can create your own auto release pool at demand, which will be put into auto release pool stack.
     */
    AutoreleasePool* getCurrentPool() const;

    bool isObjectInPools(Object* obj) const;

    friend class AutoreleasePool;

private:
    PoolManager();
    ~PoolManager();

    void push(AutoreleasePool* pool);
    void pop();

    static PoolManager* s_singleInstance;

    std::vector<AutoreleasePool*> _releasePoolStack;
};
/**
 * @endcond
 */

}

