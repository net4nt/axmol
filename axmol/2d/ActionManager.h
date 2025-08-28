/****************************************************************************
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2009      Valentin Milea
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
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

#include "axmol/2d/Action.h"
#include "axmol/base/Vector.h"
#include "axmol/base/Object.h"

namespace ax
{

struct ActionHandle
{
    Vector<Action*> actions;
    int actionIndex;
    Action* currentAction;
    bool currentActionSalvaged;
    bool paused;
};

/**
 * @addtogroup actions
 * @{
 */

/** @class ActionManager
 @brief ActionManager is a singleton that manages all the actions.
 Normally you won't need to use this singleton directly. 99% of the cases you will use the Node interface,
 which uses this singleton.
 But there are some cases where you might need to use this singleton.
 Examples:
    - When you want to run an action where the target is different from a Node.
    - When you want to pause / resume the actions.

 @since v0.8
 */
class AX_DLL ActionManager : public Object
{
public:
    /**
     */
    ActionManager();

    /**
     * @lua NA
     */
    virtual ~ActionManager();

    // actions

    /** Adds an action with a target.
     If the target is already present, then the action will be added to the existing target.
     If the target is not present, a new instance of this target will be created either paused or not, and the action
     will be added to the newly created target. When the target is paused, the queued actions won't be 'ticked'.
     *
     * @param action    A certain action.
     * @param target    The target which need to be added an action.
     * @param paused    Is the target paused or not.
     */
    virtual void addAction(Action* action, Node* target, bool paused);

    /** Removes all actions from all the targets.
     */
    virtual void removeAllActions();

    /** Removes all actions from a certain target.
     All the actions that belongs to the target will be removed.
     *
     * @param target    A certain target.
     */
    virtual void removeAllActionsFromTarget(Node* target);

    /** Removes an action given an action reference.
     *
     * @param action    A certain target.
     */
    virtual void removeAction(Action* action);

    /** Removes an action given its tag and the target.
     *
     * @param tag       The action's tag.
     * @param target    A certain target.
     */
    virtual void removeActionByTag(int tag, Node* target);

    /** Removes all actions given its tag and the target.
     *
     * @param tag       The actions' tag.
     * @param target    A certain target.
     */
    virtual void removeAllActionsByTag(int tag, Node* target);

    /** Removes all actions matching at least one bit in flags and the target.
     *
     * @param flags     The flag field to match the actions' flags based on bitwise AND.
     * @param target    A certain target.
     */
    virtual void removeActionsByFlags(unsigned int flags, Node* target);

    /** Gets an action given its tag an a target.
     *
     * @param tag       The action's tag.
     * @param target    A certain target.
     * @return  The Action the with the given tag.
     */
    virtual Action* getActionByTag(int tag, const Node* target) const;

    /** Returns the numbers of actions that are running in a certain target.
     * Composable actions are counted as 1 action. Example:
     * - If you are running 1 Sequence of 7 actions, it will return 1.
     * - If you are running 7 Sequences of 2 actions, it will return 7.
     *
     * @param target    A certain target.
     * @return  The numbers of actions that are running in a certain target.
     */
    virtual ssize_t getNumberOfRunningActionsInTarget(const Node* target) const;

    /** Returns the numbers of actions that are running in all targets.
     * @return  The numbers of actions that are running in all target.
     */
    virtual ssize_t getNumberOfRunningActions() const;

    /** Returns the numbers of actions that are running in a
     *  certain target with a specific tag.
     * Like getNumberOfRunningActionsInTarget Composable actions
     * are counted as 1 action. Example:
     * - If you are running 1 Sequence of 7 actions, it will return 1.
     * - If you are running 7 Sequences of 2 actions, it will return 7.
     *
     * @param target    A certain target.
     * @param tag       Tag that will be searched.
     * @return  The numbers of actions that are running in a certain target
     *          with a specific tag.
     * @see getNumberOfRunningActionsInTarget
     */
    virtual size_t getNumberOfRunningActionsInTargetByTag(const Node* target, int tag);

    /** Pauses the target: all running actions and newly added actions will be paused.
     *
     * @param target    A certain target.
     */
    virtual void pauseTarget(Node* target);

    /** Resumes the target. All queued actions will be resumed.
     *
     * @param target    A certain target.
     */
    virtual void resumeTarget(Node* target);

    /** Pauses all running actions, returning a list of targets whose actions were paused.
     *
     * @return  A list of targets whose actions were paused.
     */
    virtual Vector<Node*> pauseAllRunningActions();

    /** Resume a set of targets (convenience function to reverse a pauseAllRunningActions call).
     *
     * @param targetsToResume   A set of targets need to be resumed.
     */
    virtual void resumeTargets(const Vector<Node*>& targetsToResume);

    /** Main loop of ActionManager.
     * @param dt    In seconds.
     */
    virtual void update(float dt);

protected:
    // declared in ActionManager.m
    void removeTargetActionHandle(std::unordered_map<Node*, ActionHandle>::iterator& actionIt);

    void removeActionAtIndex(ssize_t index,
                             ActionHandle& element,
                             std::unordered_map<Node*, ActionHandle>::iterator actionIt);

    void reserveActionCapacity(ActionHandle& element);

    void eraseTargetActionHandle(std::unordered_map<Node*, ActionHandle>::iterator& actionIt);

protected:
    std::unordered_map<Node*, ActionHandle> _targets;
    ActionHandle* _currentTarget;
    bool _currentTargetSalvaged;
};

// end of actions group
/// @}

}  // namespace ax
