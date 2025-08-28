/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2013-2017 Chukong Technologies
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

#include "axmol/platform/PlatformMacros.h"
#include "axmol/base/Config.h"

#define AX_OBJECT_LEAK_DETECTION 0

/**
 * @addtogroup base
 * @{
 */
namespace ax
{

class Object;

/**
 * Interface that defines how to clone an Object.
 * @lua NA
 */
class AX_DLL Clonable
{
public:
    /** Returns a copy of the Object. */
    virtual Clonable* clone() const = 0;

    /**
     * @lua NA
     */
    virtual ~Clonable() {};
};

/**
 * Object is used for reference count management. If a class inherits from Object,
 * then it is easy to be shared in different places.
 */
class AX_DLL Object
{
public:
    /*
     * Take ownership newValue with retain
     */
    template <class T>
    static bool assign(T*& target, T* newValue)
    {
        if (newValue)
            newValue->retain();
        if (target)
            target->release();
        bool ret = target != newValue;
        target   = newValue;
        return ret;
    }

    /*
     * Take ownership newValue without retain
     */
    template <class T>
    static bool adopt(T*& target, T* newValue)
    {
        if (target != newValue)
        {
            if (target)
                target->release();
            target = newValue;
            return true;
        }
        return false;
    }

    /**
     * Retains the ownership.
     *
     * This increases the Object's reference count.
     *
     * @see release, autorelease
     */
    void retain();

    /**
     * Releases the ownership immediately.
     *
     * This decrements the Object's reference count.
     *
     * If the reference count reaches 0 after the decrement, this Object is
     * destructed.
     *
     * @see retain, autorelease
     */
    void release();

    /**
     * Releases the ownership sometime soon automatically.
     *
     * This decrements the Object's reference count at the end of current
     * autorelease pool block.
     *
     * If the reference count reaches 0 after the decrement, this Object is
     * destructed.
     *
     * @returns The Object itself.
     *
     * @see AutoreleasePool, retain, release
     * @lua NA
     */
    Object* autorelease();

    /**
     * Returns the Object's current reference count.
     *
     * @returns The Object's reference count.
     */
    unsigned int getReferenceCount() const;

protected:
    /**
     * Constructor
     *
     * The Object's reference count is 1 after construction.
     */
    Object();

public:
    /**
     * Destructor
     *
     * @lua NA
     */
    virtual ~Object();

protected:
    /// count of references
    unsigned int _referenceCount;

    friend class AutoreleasePool;

#if AX_ENABLE_SCRIPT_BINDING
public:
    /// object id, ScriptSupport need public _ID
    unsigned int _ID;
    /// Lua reference id
    int _luaID;
#endif

    // Memory leak diagnostic data (only included when AX_OBJECT_LEAK_DETECTION is defined and its value isn't zero)
#if AX_OBJECT_LEAK_DETECTION
public:
    static void printLeaks();
#endif
};

class Node;

typedef void (Object::*SEL_CallFunc)();
typedef void (Object::*SEL_CallFuncN)(Node*);
typedef void (Object::*SEL_CallFuncND)(Node*, void*);
typedef void (Object::*SEL_CallFuncO)(Object*);
typedef void (Object::*SEL_MenuHandler)(Object*);
typedef void (Object::*SEL_SCHEDULE)(float);

#define AX_CALLFUNC_SELECTOR(_SELECTOR)   static_cast<ax::SEL_CallFunc>(&_SELECTOR)
#define AX_CALLFUNCN_SELECTOR(_SELECTOR)  static_cast<ax::SEL_CallFuncN>(&_SELECTOR)
#define AX_CALLFUNCND_SELECTOR(_SELECTOR) static_cast<ax::SEL_CallFuncND>(&_SELECTOR)
#define AX_CALLFUNCO_SELECTOR(_SELECTOR)  static_cast<ax::SEL_CallFuncO>(&_SELECTOR)
#define AX_MENU_SELECTOR(_SELECTOR)       static_cast<ax::SEL_MenuHandler>(&_SELECTOR)
#define AX_SCHEDULE_SELECTOR(_SELECTOR)   static_cast<ax::SEL_SCHEDULE>(&_SELECTOR)

}  // namespace ax
// end of base group
/** @} */

namespace axstd
{
template <typename _Ty, typename Enable = void>
struct is_ref_counted : std::false_type
{};

template <typename... _Types>
struct is_ref_counted_helper
{};

template <typename _Ty>
struct is_ref_counted<_Ty,
                      std::conditional_t<false,
                                         is_ref_counted_helper<decltype(std::declval<_Ty>().retain()),
                                                               decltype(std::declval<_Ty>().release()),
                                                               decltype(std::declval<_Ty>().getReferenceCount())>,
                                         void>> : public std::true_type
{};

template <typename _Ty>
inline constexpr bool is_ref_counted_v = is_ref_counted<std::remove_pointer_t<_Ty>>::value;
}  // namespace axstd
