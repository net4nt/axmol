/****************************************************************************
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

#include "WebSocketTest.h"
#include "../NetworkTest.h"
#include "testResource.h"

#include "axmol/base/format.h"

/* "ws://echo.websocket.org no longer avaiable: https://www.lob.com/blog/websocket-org-is-down-here-is-an-alternative
 list of public test servers:
   - wss://echo.websocket.events/.ws
   - wss://ws.postman-echo.com/raw
   - wss://ws.postman-echo.com/socketio handshake path: random, handling
   - wss://socketsbay.com/wss/v2/1/demo/
   - https://blog.postman.com/introducing-postman-websocket-echo-service/
*/
#define ECHO_SERVER_URL "wss://echo.websocket.events/.ws" //"wss://ws.postman-echo.com/raw"
#define SOCKETIO_SERVICE "wss://ws.postman-echo.com/socketio"

using namespace ax;
using namespace ax::network;
// USING_NS_AX_EXT;

WebSocketTests::WebSocketTests()
{
    ADD_TEST_CASE(WebSocketTest);
    ADD_TEST_CASE(WebSocketCloseTest);
    ADD_TEST_CASE(WebSocketDelayTest);
}

WebSocketTest::WebSocketTest()
    : _wsiSendText(nullptr)
    , _wsiSendBinary(nullptr)
    , _wsiError(nullptr)
    , _sendTextStatus(nullptr)
    , _sendBinaryStatus(nullptr)
    , _errorStatus(nullptr)
    , _sendTextTimes(0)
    , _sendBinaryTimes(0)
{
    auto winSize = Director::getInstance()->getWinSize();

    const int MARGIN = 40;
    const int SPACE  = 35;

    auto menuRequest = Menu::create();
    menuRequest->setPosition(Vec2::ZERO);
    addChild(menuRequest);

    // Send Text
    auto labelSendText = Label::createWithTTF("Send Text", "fonts/arial.ttf", 20);
    auto itemSendText = MenuItemLabel::create(labelSendText, AX_CALLBACK_1(WebSocketTest::onMenuSendTextClicked, this));
    itemSendText->setPosition(Vec2(winSize.width / 2, winSize.height - MARGIN - SPACE));
    menuRequest->addChild(itemSendText);

    labelSendText = Label::createWithTTF("Send Multiple Text", "fonts/arial.ttf", 20);
    itemSendText =
        MenuItemLabel::create(labelSendText, AX_CALLBACK_1(WebSocketTest::onMenuSendMultipleTextClicked, this));
    itemSendText->setPosition(Vec2(winSize.width / 2, winSize.height - MARGIN - 2 * SPACE));
    menuRequest->addChild(itemSendText);

    // Send Binary
    auto labelSendBinary = Label::createWithTTF("Send Binary", "fonts/arial.ttf", 20);
    auto itemSendBinary =
        MenuItemLabel::create(labelSendBinary, AX_CALLBACK_1(WebSocketTest::onMenuSendBinaryClicked, this));
    itemSendBinary->setPosition(Vec2(winSize.width / 2, winSize.height - MARGIN - 3 * SPACE));
    menuRequest->addChild(itemSendBinary);

    // Send Text Status Label
    _sendTextStatus = Label::createWithTTF("Send Text WS is waiting...", "fonts/arial.ttf", 16, Size(160, 100),
                                           TextHAlignment::CENTER, TextVAlignment::TOP);
    _sendTextStatus->setAnchorPoint(Vec2(0, 0));
    _sendTextStatus->setPosition(Vec2(VisibleRect::left().x, VisibleRect::rightBottom().y + 25));
    this->addChild(_sendTextStatus);

    // Send Binary Status Label
    _sendBinaryStatus = Label::createWithTTF("Send Binary WS is waiting...", "fonts/arial.ttf", 16, Size(160, 100),
                                             TextHAlignment::CENTER, TextVAlignment::TOP);
    _sendBinaryStatus->setAnchorPoint(Vec2(0, 0));
    _sendBinaryStatus->setPosition(Vec2(VisibleRect::left().x + 160, VisibleRect::rightBottom().y + 25));
    this->addChild(_sendBinaryStatus);

    // Error Label
    _errorStatus = Label::createWithTTF("Error WS is waiting...", "fonts/arial.ttf", 16, Size(160, 100),
                                        TextHAlignment::CENTER, TextVAlignment::TOP);
    _errorStatus->setAnchorPoint(Vec2(0, 0));
    _errorStatus->setPosition(Vec2(VisibleRect::left().x + 320, VisibleRect::rightBottom().y + 25));
    this->addChild(_errorStatus);

    auto startTestLabel = Label::createWithTTF("Start Test WebSocket", "fonts/arial.ttf", 16);
    auto startTestItem  = MenuItemLabel::create(startTestLabel, AX_CALLBACK_1(WebSocketTest::startTestCallback, this));
    startTestItem->setPosition(Vec2(VisibleRect::center().x - 150, VisibleRect::bottom().y + 150));
    _startTestMenu = Menu::create(startTestItem, nullptr);
    _startTestMenu->setPosition(Vec2::ZERO);
    this->addChild(_startTestMenu, 1);
}

WebSocketTest::~WebSocketTest() {}

void WebSocketTest::onExit()
{
    if (_wsiSendText)
    {
        _wsiSendText->closeAsync();
        AX_SAFE_DELETE(_wsiSendText);
    }

    if (_wsiSendBinary)
    {
        _wsiSendBinary->closeAsync();
        AX_SAFE_DELETE(_wsiSendBinary);
    }

    if (_wsiError)
    {
        _wsiError->closeAsync();
        AX_SAFE_DELETE(_wsiError);
    }
    TestCase::onExit();
}

void WebSocketTest::startTestCallback(Object* sender)
{
    removeChild(_startTestMenu);
    _startTestMenu = nullptr;

    _wsiSendText   = new network::WebSocket();
    _wsiSendBinary = new network::WebSocket();
    _wsiError      = new network::WebSocket();

    if (!_wsiSendText->open(this, ECHO_SERVER_URL))
    {
        AX_SAFE_DELETE(_wsiSendText);
    }
    else
    {
        retain();  // Retain self to avoid WebSocketTest instance be deleted immediately, it will be released in
                   // WebSocketTest::onClose.
    }

    if (!_wsiSendBinary->open(this, ECHO_SERVER_URL))
    {
        AX_SAFE_DELETE(_wsiSendBinary);
    }
    else
    {
        retain();  // Retain self to avoid WebSocketTest instance be deleted immediately, it will be released in
                   // WebSocketTest::onClose.
    }

    if (!_wsiError->open(this, "ws://invalid.urlxxxxxxxx.com"))
    {
        AX_SAFE_DELETE(_wsiError);
    }
    else
    {
        retain();  // Retain self to avoid WebSocketTest instance be deleted immediately, it will be released in
                   // WebSocketTest::onClose.
    }
}

// Delegate methods
void WebSocketTest::onOpen(network::WebSocket* ws)
{
    char status[256];
    fmt::format_to_z(status, "Opened, url: {}, protocol: {}", ws->getUrl(), ws->getProtocol());

    AXLOGI("Websocket ({}) was opened, url: {}, protocol: {}", fmt::ptr(ws), ws->getUrl(), ws->getProtocol());
    if (ws == _wsiSendText)
    {
        _sendTextStatus->setString(status);
    }
    else if (ws == _wsiSendBinary)
    {
        _sendBinaryStatus->setString(status);
    }
    else if (ws == _wsiError)
    {
        AXASSERT(0, "error test will never go here.");
    }
}

void WebSocketTest::onMessage(network::WebSocket* ws, const network::WebSocket::Data& data)
{
    if (!data.isBinary)
    {
        _sendTextTimes++;
        std::string textStr = fmt::format(FMT_COMPILE("#{} response text msg: {}"), _sendTextTimes,
                                          std::string_view{data.bytes, static_cast<size_t>(data.len)});
        AXLOGI("{}", textStr);

        _sendTextStatus->setString(textStr);
    }
    else
    {
        _sendBinaryTimes++;
        char times[100];
        fmt::format_to_z(times, "{}", _sendBinaryTimes);

        std::string binaryStr = "response bin msg: ";

        for (int i = 0; i < data.len; ++i)
        {
            if (data.bytes[i] != '\0')
            {
                binaryStr += data.bytes[i];
            }
            else
            {
                binaryStr += "\'\\0\'";
            }
        }

        binaryStr += std::string(", ") + times;
        AXLOGD("{}", binaryStr);
        _sendBinaryStatus->setString(binaryStr.c_str());
    }
}

void WebSocketTest::onClose(network::WebSocket* ws, uint16_t code, std::string_view reason)
{
    AXLOGD("onClose: websocket instance ({}) closed.", fmt::ptr(ws));
    if (ws == _wsiSendText)
    {
        // _wsiSendText = nullptr;
        _sendTextStatus->setString("Send Text WS was closed");
    }
    else if (ws == _wsiSendBinary)
    {
        // _wsiSendBinary = nullptr;
        _sendBinaryStatus->setString("Send Binary WS was closed");
    }
    else if (ws == _wsiError)
    {
        // _wsiError = nullptr;
        _errorStatus->setString("Test invalid URL WS was closed");
    }
    // Delete websocket instance.
    // AX_SAFE_DELETE(ws);
    AXLOGD("WebSocketTest ref: {}", _referenceCount);
    release();
}

void WebSocketTest::onError(network::WebSocket* ws, const network::WebSocket::ErrorCode& error)
{
    AXLOGD("Error was fired, error code: {}", static_cast<int>(error));
    char buf[100];
    fmt::format_to_z(buf, "An error was fired, code: {}", static_cast<int>(error));

    if (ws == _wsiSendText)
    {
        _sendTextStatus->setString(buf);
    }
    else if (ws == _wsiSendBinary)
    {
        _sendBinaryStatus->setString(buf);
    }
    else if (ws == _wsiError)
    {
        _errorStatus->setString(buf);
    }
}

// Menu Callbacks
void WebSocketTest::onMenuSendTextClicked(ax::Object* sender)
{
    if (!_wsiSendText)
    {
        return;
    }

    if (_wsiSendText->getReadyState() == network::WebSocket::State::OPEN)
    {
        _sendTextStatus->setString("Send Text WS is waiting...");
        _wsiSendText->send("Hello WebSocket, I'm a text message from axmol-" AX_VERSION_STR_FULL " game engine.");
    }
    else
    {
        std::string warningStr = "send text websocket instance wasn't ready...";
        AXLOGD("{}", warningStr);
        _sendTextStatus->setString(warningStr.c_str());
    }
}

void WebSocketTest::onMenuSendMultipleTextClicked(ax::Object* sender)
{
    if (!_wsiSendText)
    {
        return;
    }

    if (_wsiSendText->getReadyState() == network::WebSocket::State::OPEN)
    {
        _sendTextStatus->setString("Send Multiple Text WS is waiting...");
        for (int index = 0; index < 15; ++index)
        {
            _wsiSendText->send(fmt::format("Hello WebSocket, text message index:{}", index));
        }
    }
    else
    {
        std::string warningStr = "send text websocket instance wasn't ready...";
        AXLOGD("{}", warningStr);
        _sendTextStatus->setString(warningStr.c_str());
    }
}

void WebSocketTest::onMenuSendBinaryClicked(ax::Object* sender)
{
    if (!_wsiSendBinary)
    {
        return;
    }

    if (_wsiSendBinary->getReadyState() == network::WebSocket::State::OPEN)
    {
        _sendBinaryStatus->setString("Send Binary WS is waiting...");
        char buf[] = "Hello WebSocket,\0 I'm\0 a\0 binary\0 message\0.";
        _wsiSendBinary->send(buf, sizeof(buf));
    }
    else
    {
        std::string warningStr = "send binary websocket instance wasn't ready...";
        AXLOGD("{}", warningStr);
        _sendBinaryStatus->setString(warningStr);
    }
}

WebSocketCloseTest::WebSocketCloseTest() : _wsiTest(nullptr)
{
    auto winSize = Director::getInstance()->getWinSize();

    _wsiTest = new network::WebSocket();

    if (!_wsiTest->open(this, ECHO_SERVER_URL))
    {
        delete _wsiTest;
        _wsiTest = nullptr;
    }

    auto closeItem =
        MenuItemImage::create(s_pathClose, s_pathClose, [](Object* sender) { Director::getInstance()->end(); });
    closeItem->setPosition(VisibleRect::right().x / 2, VisibleRect::top().y * 2 / 3);

    auto menu = Menu::create(closeItem, nullptr);
    menu->setPosition(Vec2::ZERO);
    addChild(menu, 1);

    auto notifyLabel = Label::createWithTTF(
        "See log window, when enter there's should have\n'Websocket opened' log,\nwhen close there's should "
        "have'websocket closed' log",
        "fonts/arial.ttf", 20);
    notifyLabel->setPosition(VisibleRect::right().x / 2, VisibleRect::top().y / 3);
    notifyLabel->setAlignment(TextHAlignment::CENTER);
    addChild(notifyLabel, 1);
}

WebSocketCloseTest::~WebSocketCloseTest()
{
    if (_wsiTest != nullptr)
    {
        _wsiTest->close();
        delete _wsiTest;
        _wsiTest = nullptr;
    }
}

// Delegate methods
void WebSocketCloseTest::onOpen(network::WebSocket* ws)
{
    AXLOGD("Websocket ({}) opened", fmt::ptr(ws));
}

void WebSocketCloseTest::onMessage(network::WebSocket* ws, const network::WebSocket::Data& data)
{
    AXLOGD("Websocket get message from {}", fmt::ptr(ws));
}

void WebSocketCloseTest::onClose(network::WebSocket* ws, uint16_t code, std::string_view reason)
{
    AXLOGD("websocket ({}) closed.", fmt::ptr(ws));
    // if (ws == _wsiTest) {
    //     _wsiTest = nullptr;
    // }
    // AX_SAFE_DELETE(ws);
}

void WebSocketCloseTest::onError(network::WebSocket* ws, const network::WebSocket::ErrorCode& error)
{
    AXLOGD("Error was fired, error code: {}", static_cast<int>(error));
}

#define SEND_TEXT_TIMES 100

WebSocketDelayTest::WebSocketDelayTest()
    : _wsiSendText(nullptr), _sendTextStatus(nullptr), _progressStatus(nullptr), _sendTextTimes(0)
{
    auto winSize = Director::getInstance()->getWinSize();

    const int MARGIN = 40;
    const int SPACE  = 35;

    auto menuRequest = Menu::create();
    menuRequest->setPosition(Vec2::ZERO);
    addChild(menuRequest);

    // Send Text
    char buf[60];
    auto cmdLabel = fmt::format_to_z(buf, "Send {} Text", SEND_TEXT_TIMES);
    auto labelSendText = Label::createWithTTF(cmdLabel, "fonts/arial.ttf", 20);
    auto itemSendText =
        MenuItemLabel::create(labelSendText, AX_CALLBACK_1(WebSocketDelayTest::onMenuSendTextClicked, this));
    itemSendText->setPosition(Vec2(winSize.width / 2, winSize.height - MARGIN - SPACE));
    menuRequest->addChild(itemSendText);

    // Send Text Status Label
    _sendTextStatus = Label::createWithTTF("Waiting connection...", "fonts/arial.ttf", 16, Size(160, 100),
                                           TextHAlignment::CENTER, TextVAlignment::TOP);
    _sendTextStatus->setAnchorPoint(Vec2(0, 0));
    _sendTextStatus->setPosition(Vec2(VisibleRect::left().x, VisibleRect::rightBottom().y + 25));
    this->addChild(_sendTextStatus);

    // Error Label
    _progressStatus =
        Label::createWithTTF(".", "fonts/arial.ttf", 16, Size(160, 100), TextHAlignment::CENTER, TextVAlignment::TOP);
    _progressStatus->setAnchorPoint(Vec2(0, 0));
    _progressStatus->setPosition(Vec2(VisibleRect::left().x + 320, VisibleRect::rightBottom().y + 25));
    this->addChild(_progressStatus);

    auto startTestLabel = Label::createWithTTF("DO Connect!", "fonts/arial.ttf", 16);
    auto startTestItem =
        MenuItemLabel::create(startTestLabel, AX_CALLBACK_1(WebSocketDelayTest::startTestCallback, this));
    startTestItem->setPosition(Vec2(VisibleRect::center().x - 150, VisibleRect::bottom().y + 150));
    _startTestMenu = Menu::create(startTestItem, nullptr);
    _startTestMenu->setPosition(Vec2::ZERO);
    this->addChild(_startTestMenu, 1);
}

WebSocketDelayTest::~WebSocketDelayTest() {}

void WebSocketDelayTest::onExit()
{
    if (_wsiSendText)
    {
        _wsiSendText->close();
        delete _wsiSendText;
        _wsiSendText = nullptr;
    }

    TestCase::onExit();
}

void WebSocketDelayTest::startTestCallback(Object* sender)
{
    removeChild(_startTestMenu);
    _startTestMenu = nullptr;

    _wsiSendText = new network::WebSocket();
    if (!_wsiSendText->open(this, ECHO_SERVER_URL))
    {
        AX_SAFE_DELETE(_wsiSendText);
    }
    else
    {
        retain();  // Retain self to avoid WebSocketDelayTest instance be deleted immediately, it will be released in
                   // WebSocketDelayTest::onClose.
    }
}

void WebSocketDelayTest::doSendText()
{
    _sendTextTimes += 1;
    if (_sendTextTimes > SEND_TEXT_TIMES)
    {
        _sendTextStatus->setString("Test Done!");
        return;
    }

    char buf[80];
    auto status = fmt::format_to_z(buf, "Sending #{}/{} text", _sendTextTimes, SEND_TEXT_TIMES);
    _sendTextStatus->setString(status);
    _sendTimeMircoSec = getNowMircroSeconds();
    _wsiSendText->send("Hello WebSocket, I'm a text message.");
}

void WebSocketDelayTest::doReceiveText()
{
    _receiveTimeMircoSec = getNowMircroSeconds();
    if (_sendTimeMircoSec > 0)
        _totalDelayMircoSec += (_receiveTimeMircoSec - _sendTimeMircoSec);
    doSendText();  // send next
}

// Delegate methods
void WebSocketDelayTest::onOpen(network::WebSocket* ws)
{
    char status[256];
    fmt::format_to_z(status, "Opened, url: {}, protocol: {}", ws->getUrl(), ws->getProtocol());

    AXLOGD("Websocket ({}) was opened, url: {}, protocol: {}", fmt::ptr(ws), ws->getUrl(), ws->getProtocol());
    if (ws == _wsiSendText)
    {
        _sendTextStatus->setString(status);
    }
}

void WebSocketDelayTest::onMessage(network::WebSocket* ws, const network::WebSocket::Data& data)
{
    if (!data.isBinary)
    {
        _receiveTextTimes++;
        char buf[100];
        auto infoStr = fmt::format_to_z(buf, "{}", _receiveTextTimes);
        std::string textStr = (std::string("response text msg: ") + data.bytes + ", ");
        textStr += infoStr;
        AXLOGD("{}", textStr);
        doReceiveText();
        infoStr = fmt::format_to_z(buf, "total delay {} seconds", (float)(_totalDelayMircoSec / 1000000.0));
        _progressStatus->setString(infoStr);
    }
}

void WebSocketDelayTest::onClose(network::WebSocket* ws, uint16_t code, std::string_view reason)
{
    AXLOGD("onClose: websocket instance ({}) closed.", fmt::ptr(ws));
    if (ws == _wsiSendText)
    {
        // delete _wsiSendText;
        // _wsiSendText = nullptr;
        _sendTextStatus->setString("Send Text WS was closed");
    }
    AXLOGD("WebSocketDelayTest ref: {}", _referenceCount);
    release();
}

void WebSocketDelayTest::onError(network::WebSocket* ws, const network::WebSocket::ErrorCode& error)
{
    AXLOGD("Error was fired, error code: {}", static_cast<int>(error));
    char buf[100];
    fmt::format_to_z(buf, "An error was fired, code: {}", static_cast<int>(error));

    if (ws == _wsiSendText)
    {
        _sendTextStatus->setString(buf);
    }

    _sendTimeMircoSec = 0;
}

// Menu Callbacks
void WebSocketDelayTest::onMenuSendTextClicked(ax::Object* sender)
{
    if (!_wsiSendText)
    {
        return;
    }

    if (_wsiSendText->getReadyState() == network::WebSocket::State::OPEN)
    {

        _sendTextTimes    = 0;
        _receiveTextTimes = 0;
        doSendText();
    }
    else
    {
        std::string warningStr = "send text websocket instance wasn't ready...";
        AXLOGD("{}", warningStr);
        _sendTextStatus->setString(warningStr);
    }
}
