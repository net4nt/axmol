/****************************************************************************
 Copyright (c) 2013      cocos2d-x.org
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

#include <string>
#include "doctest.h"
#include "AppDelegate.h"

using namespace ax;

static Vec2 gWindowSize = Vec2(1024, 768);

void AppDelegate::initGfxContextAttrs()
{
    // set graphics context attributes: red,green,blue,alpha,depth,stencil
    GfxContextAttrs gfxContextAttrs = {8, 8, 8, 8, 24, 8, 0};

    RenderView::setGfxContextAttrs(gfxContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching()
{
    // Enable logging output colored text style and prefix timestamp
    ax::setLogFmtFlag(ax::LogFmtFlag::Full);

    // whether enable global SDF font render support, since axmol-2.0.1
    FontFreeType::setShareDistanceFieldEnabled(true);

    // initialize director
    auto director   = Director::getInstance();
    auto renderView = director->getRenderView();
    if (!renderView)
    {
        std::string title = "Unit Tests";
#ifdef AX_PLATFORM_PC
        renderView = RenderViewImpl::createWithRect(title, Rect(0, 0, gWindowSize.x, gWindowSize.y), 1.0F, true);
#else
        renderView = RenderViewImpl::createWithRect(title, Rect(0, 0, gWindowSize.x, gWindowSize.y));
#endif
        director->setRenderView(renderView);
    }

    director->setStatsDisplay(true);

#ifdef AX_PLATFORM_PC
    director->setAnimationInterval(1.0f / glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate);
#else
    director->setAnimationInterval(1.0f / 60);
#endif

    auto screenSize = renderView->getFrameSize();

    renderView->setDesignResolutionSize(gWindowSize.x, gWindowSize.y, ResolutionPolicy::SHOW_ALL);

    return true;
}

void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();
}

int AppDelegate::run(int argc, char** argv)
{
    AXLOGI("Running unit tests...\n");
    fflush(stdout);
    AXLOGI("Default resource path: {}\n", FileUtils::getInstance()->getDefaultResourceRootPath());
    AXLOGI("Writable path: {}\n", FileUtils::getInstance()->getWritablePath());
    {
        for (auto& path : FileUtils::getInstance()->getSearchPaths())
            AXLOGI("Search path: {}\n", path);
    }
    fflush(stdout);

    ax::Director::getInstance()->init();

    doctest::Context context;

    // context.addFilter("test-case-exclude", "*math*"); // exclude test cases with "math" in their name
    // context.setOption("abort-after", 5);              // stop test execution after 5 failed assertions

    // context.setOption("order-by", "name");            // sort the test cases by their name

    context.applyCommandLine(argc, argv);

    // overrides
    context.setOption("no-breaks", true);  // don't break in the debugger when assertions fail

    int res = context.run();  // run
    return res;
}
