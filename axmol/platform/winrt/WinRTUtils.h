/****************************************************************************
Copyright (c) 2010 cocos2d-x.org
Copyright (c) Microsoft Open Technologies, Inc.
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

#include <string>

#include <winrt/Windows.Foundation.h>
using namespace winrt;

namespace ax
{

bool isWindowsPhone();

Windows::Foundation::IInspectable findXamlElement(Windows::Foundation::IInspectable const& parent, const winrt::hstring& name);
bool removeXamlElement(Windows::Foundation::IInspectable const& parent, Windows::Foundation::IInspectable const& element);
bool replaceXamlElement(Windows::Foundation::IInspectable const& parent,
                        Windows::Foundation::IInspectable const& add,
                        Windows::Foundation::IInspectable const& remove);

std::string PlatformStringToString(const winrt::hstring& s);
winrt::hstring PlatformStringFromString(std::string_view s);

void AX_DLL printIPAddresses();

std::string AX_DLL getDeviceIPAddresses();

std::string computeHashForFile(const std::string& filePath);

// creates a  cache file path corresponding to given source file.
// srcFilePath - source file.
// cacheFilePath - cache file path to be used to save cache.
// return true if file already exists
bool createMappedCacheFile(/*In*/ const std::string& srcFilePath, /*Out*/ std::string& cacheFilePath, /*Optional*/ const std::string& ext = "");
void destroyMappedCacheFile(const std::string& key);

}
