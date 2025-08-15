/****************************************************************************
Copyright (c) 2013-2016 Chukong Technologies Inc.
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

#ifndef _CPPTESTS_CONTROLLER_H__
#define _CPPTESTS_CONTROLLER_H__

#include <condition_variable>
#include <string>
#include <thread>
#include <atomic>

#include "axmol/platform/PlatformMacros.h"
#include "axmol/2d/ActionCoroutine.h"

class TestList;
class TestSuite;
class TestCase;

namespace ax
{
class Director;
class Touch;
class Event;
class EventListenerTouchOneByOne;
}  // namespace ax

class TestController
{
public:
    static TestController* getInstance();
    static void destroyInstance();

    ~TestController();

    void startAutoTest();
    void stopAutoTest();

    void handleCrash();

    void onEnterBackground();
    void onEnterForeground();

    bool blockTouchBegan(ax::Touch* touch, ax::Event* event);

    void setCurrTestSuite(TestSuite* testSuite) { _testSuite = testSuite; }
    TestSuite* getCurrTestSuite() { return _testSuite; }
    bool isAutoTestRunning() const { return !_stopAutoTest; }

private:
    TestController();

    ax::Coroutine traverseTestList(TestList* testList);
    ax::Coroutine traverseTestSuite(TestSuite* testSuite);
    bool checkTest(TestCase* testCase);

    bool _stopAutoTest;
    bool _isRunInBackground;

    TestList* _rootTestList;
    TestSuite* _testSuite;

    ax::Node* _autoTestRunner{nullptr};
    std::string _autoTestCaptureDirectory;

    ax::Director* _director;
    ax::EventListenerTouchOneByOne* _touchListener;

    std::string _logIndentation;
};

#endif
