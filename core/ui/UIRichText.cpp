/****************************************************************************
 Copyright (c) 2013 cocos2d-x.org
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

#include "ui/UIRichText.h"

#include <sstream>
#include <vector>
#include <locale>
#include <algorithm>
#include <regex>

#include "platform/FileUtils.h"
#include "platform/Application.h"
#include "base/EventListenerTouch.h"
#include "base/EventDispatcher.h"
#include "base/Director.h"
#include "2d/Label.h"
#include "2d/Sprite.h"
#include "base/text_utils.h"
#include "base/charconv.h"
#include "ui/UIHelper.h"

#include "fmt/compile.h"
#include "platform/SAXParser.h"

using namespace ax;
using namespace ax::ui;

class UrlTouchListenerComponent : public Component
{
public:
    static const std::string COMPONENT_NAME; /*!< component name */

    static UrlTouchListenerComponent* create(Node* parent,
                                     std::string_view url,
                                     RichText::OpenUrlHandler handleOpenUrl = nullptr)
    {
        auto* component = new UrlTouchListenerComponent(parent, url, std::move(handleOpenUrl));
        component->init();
        component->autorelease();
        return component;
    }

    explicit UrlTouchListenerComponent(Node* parent, std::string_view url, RichText::OpenUrlHandler handleOpenUrl)
        : _parent(parent), _url(url), _handleOpenUrl(std::move(handleOpenUrl))
    {
    }

    bool init() override
    {
        if (!Component::init())
            return false;

        setName(UrlTouchListenerComponent::COMPONENT_NAME);

        _touchListener               = ax::EventListenerTouchOneByOne::create();
        _touchListener->onTouchBegan = AX_CALLBACK_2(UrlTouchListenerComponent::onTouchBegan, this);
        _touchListener->onTouchEnded = AX_CALLBACK_2(UrlTouchListenerComponent::onTouchEnded, this);
        _touchListener->setSwallowTouches(true);

        Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(_touchListener, _parent);
        return true;
    }

    ~UrlTouchListenerComponent() override
    {
        Director::getInstance()->getEventDispatcher()->removeEventListener(_touchListener);
        _touchListener = nullptr;
    }

    bool onTouchBegan(Touch* touch, Event* /*event*/)
    {
        // FIXME: Node::getBoundBox() doesn't return it in local coordinates... so create one manually.
        const auto localRect = Rect(Vec2::ZERO, _parent->getContentSize());
        if (localRect.containsPoint(_parent->convertTouchToNodeSpace(touch)))
        {
            return true;
        }

        return false;
    }

    void onTouchEnded(Touch* /*touch*/, Event* /*event*/)
    {
        if (_handleOpenUrl)
        {
            _handleOpenUrl(_url);
        }
    }

    void setOpenUrlHandler(RichText::OpenUrlHandler handleOpenUrl) { _handleOpenUrl = std::move(handleOpenUrl); }

private:
    Node* _parent;  // weak ref.
    std::string _url;
    RichText::OpenUrlHandler _handleOpenUrl;
    RefPtr<EventListenerTouchOneByOne> _touchListener;  // strong ref.
};
const std::string UrlTouchListenerComponent::COMPONENT_NAME("ax_ui_UIRichText_UrlTouchListenerComponent");

bool RichElement::init(int tag, const Color32& color)
{
    _tag     = tag;
    _color   = color;
    return true;
}

bool RichElement::equalType(Type type)
{
    return (_type == type);
}

void RichElement::setColor(const Color32& color)
{
    _color = color;
}

RichElementText* RichElementText::create(int tag,
                                         const Color32& color,
                                         std::string_view text,
                                         std::string_view fontName,
                                         float fontSize,
                                         uint32_t flags,
                                         std::string_view url,
                                         const Color32& outlineColor,
                                         int outlineSize,
                                         const Color32& shadowColor,
                                         const Vec2& shadowOffset,
                                         int shadowBlurRadius,
                                         const Color32& glowColor,
                                         std::string_view id)
{
    RichElementText* element = new RichElementText();
    if (element->init(tag, color, text, fontName, fontSize, flags, url, outlineColor, outlineSize, shadowColor,
                      shadowOffset, shadowBlurRadius, glowColor, id))
    {
        element->autorelease();
        return element;
    }
    AX_SAFE_DELETE(element);
    return nullptr;
}

bool RichElementText::init(int tag,
                           const Color32& color,
                           std::string_view text,
                           std::string_view fontName,
                           float fontSize,
                           uint32_t flags,
                           std::string_view url,
                           const Color32& outlineColor,
                           int outlineSize,
                           const Color32& shadowColor,
                           const Vec2& shadowOffset,
                           int shadowBlurRadius,
                           const Color32& glowColor,
                           std::string_view id)
{
    if (RichElement::init(tag, color))
    {
        _text             = text;
        _fontName         = fontName;
        _fontSize         = fontSize;
        _flags            = flags;
        _url              = url;
        _outlineColor     = outlineColor;
        _outlineSize      = outlineSize;
        _shadowColor      = shadowColor;
        _shadowOffset     = shadowOffset;
        _shadowBlurRadius = shadowBlurRadius;
        _glowColor        = glowColor;
        _id             = id;
        return true;
    }
    return false;
}

RichElementImage* RichElementImage::create(int tag,
                                           const Color32& color,
                                           std::string_view filePath,
                                           std::string_view url,
                                           Widget::TextureResType texType,
                                           std::string_view id)
{
    RichElementImage* element = new RichElementImage();
    if (element->init(tag, color, filePath, url, texType, id))
    {
        element->autorelease();
        return element;
    }
    AX_SAFE_DELETE(element);
    return nullptr;
}

bool RichElementImage::init(int tag,
                            const Color32& color,
                            std::string_view filePath,
                            std::string_view url,
                            Widget::TextureResType texType,
                            std::string_view id)
{
    if (RichElement::init(tag, color))
    {
        _filePath    = filePath;
        _width       = -1;
        _height      = -1;
        _scaleX      = 1.0f;
        _scaleY      = 1.0f;
        _url         = url;
        _textureType = texType;
        _id          = id;
        return true;
    }
    return false;
}

void RichElementImage::setWidth(int width)
{
    _width = width;
}

void RichElementImage::setHeight(int height)
{
    _height = height;
}

void RichElementImage::setUrl(std::string_view url)
{
    _url = url;
}

void RichElementImage::setId(std::string_view id)
{
    _id = id;
}

RichElementCustomNode* RichElementCustomNode::create(int tag,
                                                     const Color32& color,
                                                     ax::Node* customNode,
                                                     std::string_view id)
{
    RichElementCustomNode* element = new RichElementCustomNode();
    if (element->init(tag, color, customNode, id))
    {
        element->autorelease();
        return element;
    }
    AX_SAFE_DELETE(element);
    return nullptr;
}

bool RichElementCustomNode::init(int tag,
                                 const Color32& color,
                                 ax::Node* customNode,
                                 std::string_view id)
{
    if (RichElement::init(tag, color))
    {
        _customNode = customNode;
        _id         = id;
        _customNode->retain();
        return true;
    }
    return false;
}

RichElementNewLine* RichElementNewLine::create(int tag, const Color32& color)
{
    RichElementNewLine* element = new RichElementNewLine();
    if (element->init(tag, color))
    {
        element->autorelease();
        return element;
    }
    AX_SAFE_DELETE(element);
    return nullptr;
}

RichElementNewLine* RichElementNewLine::create(int tag, int quantity, const Color32& color)
{
    RichElementNewLine* element = new RichElementNewLine(quantity);
    if (element->init(tag, color))
    {
        element->autorelease();
        return element;
    }
    AX_SAFE_DELETE(element);
    return nullptr;
}

/** @brief parse a XML. */
class MyXMLVisitor : public SAXDelegator
{
public:
    /** @brief underline or strikethrough */
    enum class StyleLine
    {
        NONE,
        UNDERLINE,    /*!< underline */
        STRIKETHROUGH /*!< a typographical presentation of words with a horizontal line through their center */
    };

    /** @brief outline, shadow or glow */
    enum class StyleEffect
    {
        NONE,
        OUTLINE, /*!< outline effect enabled */
        SHADOW,  /*!< shadow effect enabled */
        GLOW /*!< glow effect enabled @discussion Limiting use to only when the Label created with true type font. */
    };

    /** @brief the attributes of text tag */
    struct Attributes
    {
        std::string face;     /*!< font name */
        std::string url;      /*!< url is a attribute of a anchor tag */
        float fontSize;       /*!< font size */
        Color32 color;        /*!< font color */
        bool hasColor;        /*!< or color is specified? */
        bool bold;            /*!< bold text */
        bool italics;         /*!< italic text */
        StyleLine line;       /*!< underline or strikethrough */
        StyleEffect effect;   /*!< outline, shadow or glow */
        Color32 outlineColor; /*!< the color of the outline */
        int outlineSize;      /*!< the outline effect size value */
        Color32 shadowColor;  /*!< the shadow effect color value */
        Vec2 shadowOffset;    /*!< shadow effect offset value */
        int shadowBlurRadius; /*!< the shadow effect blur radius */
        Color32 glowColor;    /*!< the glow effect color value */
        std::string name;     /*!< the name of this element */

        void setColor(const Color32& acolor)
        {
            color    = acolor;
            hasColor = true;
        }
        Attributes()
            : fontSize(-1)
            , hasColor(false)
            , bold(false)
            , italics(false)
            , line(StyleLine::NONE)
            , effect(StyleEffect::NONE)
            , outlineSize(0)
            , shadowBlurRadius(0)
        {}
    };

private:
    std::vector<Attributes> _fontElements;

    RichText* _richText;

    struct TagBehavior
    {
        bool isFontElement;
        RichText::VisitEnterHandler handleVisitEnter;
        RichText::VisitExitHandler handleVisitExit;
    };
    typedef hlookup::string_map<TagBehavior> TagTables;

    static TagTables _tagTables;

public:
    explicit MyXMLVisitor(RichText* richText);
    ~MyXMLVisitor() override;

    Color32 getColor() const;

    float getFontSize() const;

    std::string getFace() const;

    std::string getURL() const;

    bool getBold() const;

    bool getItalics() const;

    bool getUnderline() const;

    bool getStrikethrough() const;

    std::tuple<bool, Color32, int> getOutline() const;

    std::tuple<bool, Color32, Vec2, int> getShadow() const;

    std::tuple<bool, Color32> getGlow() const;

    std::string getName() const;

    void startElement(void* ctx, const char* name, const char** atts) override;

    void endElement(void* ctx, const char* name) override;

    void textHandler(void* ctx, const char* s, size_t len) override;

    void pushBackFontElement(const Attributes& attribs);

    void popBackFontElement();

    void pushBackElement(RichElement* element);

    static void setTagDescription(std::string_view tag,
                                  bool isFontElement,
                                  RichText::VisitEnterHandler&& handleVisitEnter,
                                  RichText::VisitExitHandler&& handleVisitExit = nullptr);

    static void removeTagDescription(std::string_view tag);

private:
    ValueMap tagAttrMapWithXMLElement(const char** attrs);
};

MyXMLVisitor::TagTables MyXMLVisitor::_tagTables;

MyXMLVisitor::MyXMLVisitor(RichText* richText) : _fontElements(20), _richText(richText)
{
    MyXMLVisitor::setTagDescription("font", true, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        // size, color, align, face
        ValueMap attrValueMap;

        if (auto&& itr = tagAttrValueMap.find("size"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_SIZE] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_COLOR_STRING] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("face"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_FACE] = itr->second.asString();
        }

        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("b", true, [](const ValueMap& /*tagAttrValueMap*/) {
        // no supported attributes
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_TEXT_BOLD] = true;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("i", true, [](const ValueMap& /*tagAttrValueMap*/) {
        // no supported attributes
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_TEXT_ITALIC] = true;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("del", true, [](const ValueMap& /*tagAttrValueMap*/) {
        // no supported attributes
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_TEXT_LINE] = RichText::VALUE_TEXT_LINE_DEL;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("u", true, [](const ValueMap& /*tagAttrValueMap*/) {
        // no supported attributes
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_TEXT_LINE] = RichText::VALUE_TEXT_LINE_UNDER;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("small", true, [](const ValueMap& /*tagAttrValueMap*/) {
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_FONT_SMALL] = true;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("big", true, [](const ValueMap& /*tagAttrValueMap*/) {
        ValueMap attrValueMap;
        attrValueMap[RichText::KEY_FONT_BIG] = true;
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("img", false, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        // src, height, width, scaleX, scaleY, scale
        std::string src;
        int height                     = -1;
        int width                      = -1;
        float scaleX                   = 1.f;
        float scaleY                   = 1.f;
        Widget::TextureResType resType = Widget::TextureResType::LOCAL;

        auto it = tagAttrValueMap.find("src");
        if (it != tagAttrValueMap.end())
        {
            src = it->second.asString();
        }

        it = tagAttrValueMap.find("height");
        if (it != tagAttrValueMap.end())
        {
            auto str = it->second.asStringRef();
            if (!str.empty() && str[str.length() - 1] == '%')
            {
                scaleY = std::atoi(str.data()) / 100.f;
            }
            else
            {
                height = it->second.asInt();
            }
        }

        it = tagAttrValueMap.find("width");
        if (it != tagAttrValueMap.end())
        {
            auto str = it->second.asStringRef();
            if (!str.empty() && str[str.length() - 1] == '%')
            {
                scaleX = std::atoi(str.data()) / 100.f;
            }
            else
            {
                width = it->second.asInt();
            }
        }

        it = tagAttrValueMap.find("scaleX");
        if (it != tagAttrValueMap.end())
        {
            scaleX = it->second.asFloat();
        }

        it = tagAttrValueMap.find("scaleY");
        if (it != tagAttrValueMap.end())
        {
            scaleY = it->second.asFloat();
        }

        it = tagAttrValueMap.find("scale");
        if (it != tagAttrValueMap.end())
        {
            scaleX = scaleY = it->second.asFloat();
        }

        it = tagAttrValueMap.find("type");
        if (it != tagAttrValueMap.end())
        {
            // texture type
            // 0: normal file path
            // 1: sprite frame name
            int type = it->second.asInt();
            resType  = type == 0 ? Widget::TextureResType::LOCAL : Widget::TextureResType::PLIST;
        }

        RichElementImage* elementImg = nullptr;
        if (!src.empty())
        {
            elementImg = RichElementImage::create(0, Color32::WHITE, src, "", resType);
            if (height >= 0)
                elementImg->setHeight(height);
            if (width >= 0)
                elementImg->setWidth(width);

            elementImg->setScaleX(scaleX);
            elementImg->setScaleY(scaleY);

            if (tagAttrValueMap.find("id") != tagAttrValueMap.end())
            {
                elementImg->setId(tagAttrValueMap.at("id").asString());
            }
        }
        return make_pair(ValueMap(), elementImg);
    });

    MyXMLVisitor::setTagDescription("a", true, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        ValueMap attrValueMap;

        if (auto&& itr = tagAttrValueMap.find("href"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_URL] = itr->second.asString();
        }

        if (auto&& itr = tagAttrValueMap.find("id"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_ID] = itr->second.asString();
        }

        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("br", false, [](const ValueMap& /*tagAttrValueMap*/) {
        RichElementNewLine* richElement = RichElementNewLine::create(0, Color32::WHITE);
        return make_pair(ValueMap(), richElement);
    });

    MyXMLVisitor::setTagDescription(
        "p", true,
        [](const ValueMap& tagAttrValueMap) -> std::pair<ValueMap, RichElement*> {
            ValueMap attrValueMap;
            if (auto&& itr = tagAttrValueMap.find("size"); itr != tagAttrValueMap.end())
            {
                attrValueMap[RichText::KEY_FONT_SIZE] = itr->second.asString();
            }

            if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
            {
                attrValueMap[RichText::KEY_FONT_COLOR_STRING] = itr->second.asString();
            }

            if (auto&& itr = tagAttrValueMap.find("face"); itr != tagAttrValueMap.end())
            {
                attrValueMap[RichText::KEY_FONT_FACE] = itr->second.asString();
            }

            if (auto&& itr = tagAttrValueMap.find("id"); itr != tagAttrValueMap.end())
            {
                attrValueMap[RichText::KEY_ID] = itr->second.asString();
            }

            return make_pair(attrValueMap, nullptr);
        },
        [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    constexpr auto headerTagEnterHandler = [](const ValueMap& tagAttrValueMap,
                                              std::string_view defaultFontSize) -> std::pair<ValueMap, RichElement*> {
        ValueMap attrValueMap;
        if (auto&& itr = tagAttrValueMap.find("size"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_SIZE] = itr->second.asString();
        }
        else
        {
            attrValueMap[RichText::KEY_FONT_SIZE] = defaultFontSize;
        }

        if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_COLOR_STRING] = itr->second.asString();
        }

        if (auto&& itr = tagAttrValueMap.find("face"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_FONT_FACE] = itr->second.asString();
        }

        attrValueMap[RichText::KEY_TEXT_BOLD] = true;

        if (auto&& itr = tagAttrValueMap.find("id"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_ID] = itr->second.asString();
        }

        return make_pair(attrValueMap, nullptr);
    };

    MyXMLVisitor::setTagDescription("h1", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "2em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("h2", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "1.75em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("h3", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "1.5em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("h4", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "1.25em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("h5", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "1.125em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("h6", true, [headerTagEnterHandler](const ValueMap& tagAttrValueMap) {
        return headerTagEnterHandler(tagAttrValueMap, "1em");
    }, [] { return RichElementNewLine::create(0, 2, Color32::WHITE); });

    MyXMLVisitor::setTagDescription("outline", true, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        // color, size
        ValueMap attrValueMap;

        attrValueMap[RichText::KEY_TEXT_STYLE] = RichText::VALUE_TEXT_STYLE_OUTLINE;
        if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_OUTLINE_COLOR] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("size"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_OUTLINE_SIZE] = itr->second.asString();
        }
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("shadow", true, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        // color, offsetWidth, offsetHeight, blurRadius
        ValueMap attrValueMap;

        attrValueMap[RichText::KEY_TEXT_STYLE] = RichText::VALUE_TEXT_STYLE_SHADOW;
        if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_SHADOW_COLOR] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("offsetWidth"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_SHADOW_OFFSET_WIDTH] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("offsetHeight"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_SHADOW_OFFSET_HEIGHT] = itr->second.asString();
        }
        if (auto&& itr = tagAttrValueMap.find("blurRadius"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_SHADOW_BLUR_RADIUS] = itr->second.asString();
        }
        return make_pair(attrValueMap, nullptr);
    });

    MyXMLVisitor::setTagDescription("glow", true, [](const ValueMap& tagAttrValueMap) {
        // supported attributes:
        // color
        ValueMap attrValueMap;

        attrValueMap[RichText::KEY_TEXT_STYLE] = RichText::VALUE_TEXT_STYLE_GLOW;
        if (auto&& itr = tagAttrValueMap.find("color"); itr != tagAttrValueMap.end())
        {
            attrValueMap[RichText::KEY_TEXT_GLOW_COLOR] = itr->second.asString();
        }
        return make_pair(attrValueMap, nullptr);
    });
}

MyXMLVisitor::~MyXMLVisitor() {}

Color32 MyXMLVisitor::getColor() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->hasColor)
            return i->color;
    }
    return Color32::WHITE;
}

float MyXMLVisitor::getFontSize() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->fontSize != -1)
            return i->fontSize;
    }
    return 12;
}

std::string MyXMLVisitor::getFace() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (!i->face.empty())
            return i->face;
    }
    return "fonts/Marker Felt.ttf";
}

std::string MyXMLVisitor::getURL() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (!i->url.empty())
            return i->url;
    }
    return "";
}

bool MyXMLVisitor::getBold() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->bold)
            return true;
    }
    return false;
}

bool MyXMLVisitor::getItalics() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->italics)
            return true;
    }
    return false;
}

bool MyXMLVisitor::getUnderline() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->line == StyleLine::UNDERLINE)
            return true;
    }
    return false;
}

bool MyXMLVisitor::getStrikethrough() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->line == StyleLine::STRIKETHROUGH)
            return true;
    }
    return false;
}

std::tuple<bool, Color32, int> MyXMLVisitor::getOutline() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->effect == StyleEffect::OUTLINE)
            return std::make_tuple(true, i->outlineColor, i->outlineSize);
    }
    return std::make_tuple(false, Color32::WHITE, -1);
}

std::tuple<bool, Color32, Vec2, int> MyXMLVisitor::getShadow() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->effect == StyleEffect::SHADOW)
            return std::make_tuple(true, i->shadowColor, i->shadowOffset, i->shadowBlurRadius);
    }
    return std::make_tuple(false, Color32::BLACK, Vec2(2.0, -2.0), 0);
}

std::tuple<bool, Color32> MyXMLVisitor::getGlow() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (i->effect == StyleEffect::GLOW)
            return std::make_tuple(true, i->glowColor);
    }
    return std::make_tuple(false, Color32::WHITE);
}

std::string MyXMLVisitor::getName() const
{
    for (auto i = _fontElements.rbegin(), iRend = _fontElements.rend(); i != iRend; ++i)
    {
        if (!i->name.empty())
            return i->name;
    }
    return "";
}

void MyXMLVisitor::startElement(void* /*ctx*/, const char* elementName, const char** atts)
{
    auto it = _tagTables.find(elementName);
    if (it != _tagTables.end())
    {
        auto tagBehavior = it->second;
        if (tagBehavior.handleVisitEnter != nullptr)
        {
            ValueMap&& tagAttrValueMap = tagAttrMapWithXMLElement(atts);
            auto result                = tagBehavior.handleVisitEnter(tagAttrValueMap);
            ValueMap& attrValueMap     = result.first;
            RichElement* richElement   = result.second;
            if (tagBehavior.isFontElement)
            {
                Attributes attributes;

                if (auto&& itr = attrValueMap.find(RichText::KEY_FONT_SIZE); itr != attrValueMap.end())
                {
                    std::regex fontSizePattern(R"(([0-9]*(?:\.[0-9]+)?)(%|em)$)");
                    std::smatch match;
                    auto sizeString = itr->second.asString();
                    if (std::regex_match(sizeString, match, fontSizePattern) && match.size() == 3 &&
                        !match[1].str().empty())
                    {
                        auto scale = static_cast<float>(utils::atof(match[1].str()));
                        if (match[2].str() == "%")
                        {
                            attributes.fontSize = getFontSize() * scale / 100.f;
                        }
                        else // em
                        {
                            attributes.fontSize = getFontSize() * scale;
                        }
                    }
                    else
                    {
                        attributes.fontSize = itr->second.asFloat();
                    }
                }
                if (attrValueMap.contains(RichText::KEY_FONT_SMALL))
                {
                    attributes.fontSize = getFontSize() * 0.8f;
                }
                if (attrValueMap.contains(RichText::KEY_FONT_BIG))
                {
                    attributes.fontSize = getFontSize() * 1.25f;
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_FONT_COLOR_STRING); itr != attrValueMap.end())
                {
                    attributes.setColor(_richText->RichText::parseColor32(itr->second.asString()));
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_FONT_FACE); itr != attrValueMap.end())
                {
                    attributes.face = itr->second.asString();
                }
                if (attrValueMap.contains(RichText::KEY_TEXT_BOLD))
                {
                    attributes.bold = true;
                }
                if (attrValueMap.contains(RichText::KEY_TEXT_ITALIC))
                {
                    attributes.italics = true;
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_TEXT_LINE); itr != attrValueMap.end())
                {
                    auto keyTextLine = itr->second.asString();
                    if (keyTextLine == RichText::VALUE_TEXT_LINE_DEL)
                    {
                        attributes.line = StyleLine::STRIKETHROUGH;
                    }
                    else if (keyTextLine == RichText::VALUE_TEXT_LINE_UNDER)
                    {
                        attributes.line = StyleLine::UNDERLINE;
                    }
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_URL); itr != attrValueMap.end())
                {
                    attributes.url = itr->second.asString();
                    attributes.setColor(_richText->getAnchorFontColor32());
                    if (_richText->isAnchorTextBoldEnabled())
                    {
                        attributes.bold = true;
                    }
                    if (_richText->isAnchorTextItalicEnabled())
                    {
                        attributes.italics = true;
                    }
                    if (_richText->isAnchorTextUnderlineEnabled())
                    {
                        attributes.line = StyleLine::UNDERLINE;
                    }
                    if (_richText->isAnchorTextDelEnabled())
                    {
                        attributes.line = StyleLine::STRIKETHROUGH;
                    }
                    if (_richText->isAnchorTextOutlineEnabled())
                    {
                        attributes.effect       = StyleEffect::OUTLINE;
                        attributes.outlineColor = _richText->getAnchorTextOutlineColor32();
                        attributes.outlineSize  = _richText->getAnchorTextOutlineSize();
                    }
                    if (_richText->isAnchorTextShadowEnabled())
                    {
                        attributes.effect           = StyleEffect::SHADOW;
                        attributes.shadowColor      = _richText->getAnchorTextShadowColor32();
                        attributes.shadowOffset     = _richText->getAnchorTextShadowOffset();
                        attributes.shadowBlurRadius = _richText->getAnchorTextShadowBlurRadius();
                    }
                    if (_richText->isAnchorTextGlowEnabled())
                    {
                        attributes.effect    = StyleEffect::GLOW;
                        attributes.glowColor = _richText->getAnchorTextGlowColor32();
                    }
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_TEXT_STYLE); itr != attrValueMap.end())
                {
                    auto keyTextStyle = itr->second.asString();
                    if (keyTextStyle == RichText::VALUE_TEXT_STYLE_OUTLINE)
                    {
                        attributes.effect = StyleEffect::OUTLINE;
                        if (auto&& styleItr = attrValueMap.find(RichText::KEY_TEXT_OUTLINE_COLOR);
                            styleItr != attrValueMap.end())
                        {
                            attributes.outlineColor = _richText->RichText::parseColor32(styleItr->second.asString());
                        }
                        if (auto&& styleItr = attrValueMap.find(RichText::KEY_TEXT_OUTLINE_SIZE); styleItr != attrValueMap.end())
                        {
                            attributes.outlineSize = styleItr->second.asInt();
                        }
                    }
                    else if (keyTextStyle == RichText::VALUE_TEXT_STYLE_SHADOW)
                    {
                        attributes.effect = StyleEffect::SHADOW;
                        if (auto&& styleItr = attrValueMap.find(RichText::KEY_TEXT_SHADOW_COLOR); styleItr != attrValueMap.end())
                        {
                            attributes.shadowColor = RichText::parseColor32(styleItr->second.asString());
                        }

                        auto&& shadowOffsetWidthItr = attrValueMap.find(RichText::KEY_TEXT_SHADOW_OFFSET_WIDTH);
                        auto&& shadowOffsetHeightItr = attrValueMap.find(RichText::KEY_TEXT_SHADOW_OFFSET_HEIGHT);
                        if (shadowOffsetWidthItr != attrValueMap.end() && shadowOffsetHeightItr != attrValueMap.end())
                        {
                            attributes.shadowOffset =
                                Vec2(shadowOffsetWidthItr->second.asFloat(), shadowOffsetHeightItr->second.asFloat());
                        }

                        if (auto&& styleItr =
                                attrValueMap.find(RichText::KEY_TEXT_SHADOW_BLUR_RADIUS); styleItr != attrValueMap.end())
                        {
                            attributes.shadowBlurRadius = styleItr->second.asInt();
                        }
                    }
                    else if (keyTextStyle == RichText::VALUE_TEXT_STYLE_GLOW)
                    {
                        attributes.effect = StyleEffect::GLOW;
                        if (auto&& styleItr = attrValueMap.find(RichText::KEY_TEXT_GLOW_COLOR); styleItr != attrValueMap.end())
                        {
                            attributes.glowColor = RichText::parseColor32(styleItr->second.asString());
                        }
                    }
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_URL); itr != attrValueMap.end())
                {
                    attributes.url = itr->second.asString();
                    attributes.setColor(_richText->getAnchorFontColor32());
                    if (_richText->isAnchorTextBoldEnabled())
                    {
                        attributes.bold = true;
                    }
                    if (_richText->isAnchorTextItalicEnabled())
                    {
                        attributes.italics = true;
                    }
                    if (_richText->isAnchorTextUnderlineEnabled())
                    {
                        attributes.line = StyleLine::UNDERLINE;
                    }
                    if (_richText->isAnchorTextDelEnabled())
                    {
                        attributes.line = StyleLine::STRIKETHROUGH;
                    }
                    if (_richText->isAnchorTextOutlineEnabled())
                    {
                        attributes.effect       = StyleEffect::OUTLINE;
                        attributes.outlineColor = _richText->getAnchorTextOutlineColor32();
                        attributes.outlineSize  = _richText->getAnchorTextOutlineSize();
                    }
                    if (_richText->isAnchorTextShadowEnabled())
                    {
                        attributes.effect           = StyleEffect::SHADOW;
                        attributes.shadowColor      = _richText->getAnchorTextShadowColor32();
                        attributes.shadowOffset     = _richText->getAnchorTextShadowOffset();
                        attributes.shadowBlurRadius = _richText->getAnchorTextShadowBlurRadius();
                    }
                    if (_richText->isAnchorTextGlowEnabled())
                    {
                        attributes.effect    = StyleEffect::GLOW;
                        attributes.glowColor = _richText->getAnchorTextGlowColor32();
                    }
                }
                if (auto&& itr = attrValueMap.find(RichText::KEY_ID); itr != attrValueMap.end())
                {
                    attributes.name = itr->second.asString();
                }
                pushBackFontElement(attributes);
            }
            if (richElement)
            {
                if (richElement->equalType(RichElement::Type::IMAGE))
                {
                    richElement->setColor(getColor());
                    auto* richElementImage = static_cast<RichElementImage*>(richElement);
                    richElementImage->setUrl(getURL());
                }
                else if (richElement->equalType(RichElement::Type::NEWLINE))
                {
                    richElement->setColor(getColor());
                }
                pushBackElement(richElement);
            }
        }
    }
}

void MyXMLVisitor::endElement(void* /*ctx*/, const char* elementName)
{
    auto it = _tagTables.find(elementName);
    if (it != _tagTables.end())
    {
        auto tagBehavior = it->second;
        if (tagBehavior.isFontElement)
        {
            popBackFontElement();
        }

        if (tagBehavior.handleVisitExit != nullptr)
        {
            auto* richElement = tagBehavior.handleVisitExit();
            if (richElement)
            {
                pushBackElement(richElement);
            }
        }
    }
}

void MyXMLVisitor::textHandler(void* /*ctx*/, const char* str, size_t len)
{
    std::string text(str, len);
    auto color         = getColor();
    auto face          = getFace();
    auto fontSize      = getFontSize();
    auto italics       = getItalics();
    auto underline     = getUnderline();
    auto strikethrough = getStrikethrough();
    auto bold          = getBold();
    auto url           = getURL();
    auto outline       = getOutline();
    auto shadow        = getShadow();
    auto glow          = getGlow();
    auto name          = getName();

    uint32_t flags = 0;
    if (italics)
        flags |= RichElementText::ITALICS_FLAG;
    if (bold)
        flags |= RichElementText::BOLD_FLAG;
    if (underline)
        flags |= RichElementText::UNDERLINE_FLAG;
    if (strikethrough)
        flags |= RichElementText::STRIKETHROUGH_FLAG;
    if (!url.empty())
        flags |= RichElementText::URL_FLAG;
    if (std::get<0>(outline))
        flags |= RichElementText::OUTLINE_FLAG;
    if (std::get<0>(shadow))
        flags |= RichElementText::SHADOW_FLAG;
    if (std::get<0>(glow))
        flags |= RichElementText::GLOW_FLAG;

    auto element = RichElementText::create(0, color, text, face, fontSize, flags, url, std::get<1>(outline),
                                           std::get<2>(outline), std::get<1>(shadow), std::get<2>(shadow),
                                           std::get<3>(shadow), std::get<1>(glow), name);
    _richText->pushBackElement(element);
}

void MyXMLVisitor::pushBackFontElement(const MyXMLVisitor::Attributes& attribs)
{
    _fontElements.emplace_back(attribs);
}

void MyXMLVisitor::popBackFontElement()
{
    _fontElements.pop_back();
}

void MyXMLVisitor::pushBackElement(RichElement* element)
{
    _richText->pushBackElement(element);
}

void MyXMLVisitor::setTagDescription(std::string_view tag,
                                     bool isFontElement,
                                     RichText::VisitEnterHandler&& handleVisitEnter,
                                     RichText::VisitExitHandler&& handleVisitExit)
{
    // MyXMLVisitor::_tagTables[tag] = {isFontElement, std::move(handleVisitEnter), std::move(handleVisitExit)};
    hlookup::set_item(MyXMLVisitor::_tagTables, tag,
                      TagBehavior{isFontElement, std::move(handleVisitEnter), std::move(handleVisitExit)});
}

void MyXMLVisitor::removeTagDescription(std::string_view tag)
{
    MyXMLVisitor::_tagTables.erase(tag);
}

ValueMap MyXMLVisitor::tagAttrMapWithXMLElement(const char** attrs)
{
    ValueMap tagAttrValueMap;
    for (const char** attr = attrs; *attr != nullptr; attr = (attrs += 2))
    {
        if (attr[0] && attr[1])
        {
            tagAttrValueMap[attr[0]] = attr[1];
        }
    }
    return tagAttrValueMap;
}

const std::string RichText::KEY_VERTICAL_SPACE("KEY_VERTICAL_SPACE");
const std::string RichText::KEY_WRAP_MODE("KEY_WRAP_MODE");
const std::string RichText::KEY_HORIZONTAL_ALIGNMENT("KEY_HORIZONTAL_ALIGNMENT");
const std::string RichText::KEY_VERTICAL_ALIGNMENT("KEY_VERTICAL_ALIGNMENT");
const std::string RichText::KEY_FONT_COLOR_STRING("KEY_FONT_COLOR_STRING");
const std::string RichText::KEY_FONT_SIZE("KEY_FONT_SIZE");
const std::string RichText::KEY_FONT_SMALL("KEY_FONT_SMALL");
const std::string RichText::KEY_FONT_BIG("KEY_FONT_BIG");
const std::string RichText::KEY_FONT_FACE("KEY_FONT_FACE");
const std::string RichText::KEY_TEXT_BOLD("KEY_TEXT_BOLD");
const std::string RichText::KEY_TEXT_ITALIC("KEY_TEXT_ITALIC");
const std::string RichText::KEY_TEXT_LINE("KEY_TEXT_LINE");
const std::string RichText::VALUE_TEXT_LINE_NONE("VALUE_TEXT_LINE_NONE");
const std::string RichText::VALUE_TEXT_LINE_DEL("VALUE_TEXT_LINE_DEL");
const std::string RichText::VALUE_TEXT_LINE_UNDER("VALUE_TEXT_LINE_UNDER");
const std::string RichText::KEY_TEXT_STYLE("KEY_TEXT_STYLE");
const std::string RichText::VALUE_TEXT_STYLE_NONE("VALUE_TEXT_STYLE_NONE");
const std::string RichText::VALUE_TEXT_STYLE_OUTLINE("VALUE_TEXT_STYLE_OUTLINE");
const std::string RichText::VALUE_TEXT_STYLE_SHADOW("VALUE_TEXT_STYLE_SHADOW");
const std::string RichText::VALUE_TEXT_STYLE_GLOW("VALUE_TEXT_STYLE_GLOW");
const std::string RichText::KEY_TEXT_OUTLINE_COLOR("KEY_TEXT_OUTLINE_COLOR");
const std::string RichText::KEY_TEXT_OUTLINE_SIZE("KEY_TEXT_OUTLINE_SIZE");
const std::string RichText::KEY_TEXT_SHADOW_COLOR("KEY_TEXT_SHADOW_COLOR");
const std::string RichText::KEY_TEXT_SHADOW_OFFSET_WIDTH("KEY_TEXT_SHADOW_OFFSET_WIDTH");
const std::string RichText::KEY_TEXT_SHADOW_OFFSET_HEIGHT("KEY_TEXT_SHADOW_OFFSET_HEIGHT");
const std::string RichText::KEY_TEXT_SHADOW_BLUR_RADIUS("KEY_TEXT_SHADOW_BLUR_RADIUS");
const std::string RichText::KEY_TEXT_GLOW_COLOR("KEY_TEXT_GLOW_COLOR");
const std::string RichText::KEY_URL("KEY_URL");
const std::string RichText::KEY_ANCHOR_FONT_COLOR_STRING("KEY_ANCHOR_FONT_COLOR_STRING");
const std::string RichText::KEY_ANCHOR_TEXT_BOLD("KEY_ANCHOR_TEXT_BOLD");
const std::string RichText::KEY_ANCHOR_TEXT_ITALIC("KEY_ANCHOR_TEXT_ITALIC");
const std::string RichText::KEY_ANCHOR_TEXT_LINE("KEY_ANCHOR_TEXT_LINE");
const std::string RichText::KEY_ANCHOR_TEXT_STYLE("KEY_ANCHOR_TEXT_STYLE");
const std::string RichText::KEY_ANCHOR_TEXT_OUTLINE_COLOR("KEY_ANCHOR_TEXT_OUTLINE_COLOR");
const std::string RichText::KEY_ANCHOR_TEXT_OUTLINE_SIZE("KEY_ANCHOR_TEXT_OUTLINE_SIZE");
const std::string RichText::KEY_ANCHOR_TEXT_SHADOW_COLOR("KEY_ANCHOR_TEXT_SHADOW_COLOR");
const std::string RichText::KEY_ANCHOR_TEXT_SHADOW_OFFSET_WIDTH("KEY_ANCHOR_TEXT_SHADOW_OFFSET_WIDTH");
const std::string RichText::KEY_ANCHOR_TEXT_SHADOW_OFFSET_HEIGHT("KEY_ANCHOR_TEXT_SHADOW_OFFSET_HEIGHT");
const std::string RichText::KEY_ANCHOR_TEXT_SHADOW_BLUR_RADIUS("KEY_ANCHOR_TEXT_SHADOW_BLUR_RADIUS");
const std::string RichText::KEY_ANCHOR_TEXT_GLOW_COLOR("KEY_ANCHOR_TEXT_GLOW_COLOR");
const std::string RichText::KEY_ID("KEY_ID");

RichText::RichText() : _formatTextDirty(true), _leftSpaceWidth(0.0f)
{
    _defaults[KEY_VERTICAL_SPACE]           = 0.0f;
    _defaults[KEY_WRAP_MODE]                = static_cast<int>(WrapMode::WRAP_PER_WORD);
    _defaults[KEY_HORIZONTAL_ALIGNMENT]     = static_cast<int>(HorizontalAlignment::LEFT);
    _defaults[KEY_VERTICAL_ALIGNMENT]       = static_cast<int>(VerticalAlignment::BOTTOM);
    _defaults[KEY_FONT_COLOR_STRING]        = "#ffffff";
    _defaults[KEY_FONT_SIZE]                = 12.0f;
    _defaults[KEY_FONT_FACE]                = "Verdana";
    _defaults[KEY_ANCHOR_FONT_COLOR_STRING] = "#0000FF";
    _defaults[KEY_ANCHOR_TEXT_BOLD]         = false;
    _defaults[KEY_ANCHOR_TEXT_ITALIC]       = false;
    _defaults[KEY_ANCHOR_TEXT_LINE]         = VALUE_TEXT_LINE_NONE;
    _defaults[KEY_ANCHOR_TEXT_STYLE]        = VALUE_TEXT_STYLE_NONE;
}

RichText::~RichText()
{
    _richElements.clear();
}

RichText* RichText::create()
{
    RichText* widget = new RichText();
    if (widget->init())
    {
        widget->autorelease();
        return widget;
    }
    AX_SAFE_DELETE(widget);
    return nullptr;
}

RichText* RichText::createWithXML(std::string_view xml, const ValueMap& defaults, const OpenUrlHandler& handleOpenUrl)
{
    RichText* widget = new RichText();
    if (widget->initWithXML(xml, defaults, handleOpenUrl))
    {
        widget->autorelease();
        return widget;
    }
    AX_SAFE_DELETE(widget);
    return nullptr;
}

bool RichText::init()
{
    if (Widget::init())
    {
        return true;
    }
    return false;
}

bool RichText::initWithXML(std::string_view origxml, const ValueMap& defaults, const OpenUrlHandler& handleOpenUrl)
{
    if (Widget::init())
    {
        setDefaults(defaults);
        setOpenUrlHandler(handleOpenUrl);

        return this->setString(origxml);
    }
    return false;
}

bool RichText::setString(std::string_view text)
{
    if (_text != text)
    {
        _formatTextDirty = true;

        _richElements.clear();
        _text = text;

        // solves to issues:
        //  - creates defaults values
        //  - makes sure that the xml well formed and starts with an element
        _xmlText.clear();
        fmt::format_to(std::back_inserter(_xmlText), FMT_COMPILE(R"(<font face="{}" size="{}" color="{}">{}</font>)"),
                       this->getFontFace(), this->getFontSize(), this->getFontColor(), _text);

        MyXMLVisitor visitor(this);
        SAXParser parser;
        parser.setDelegator(&visitor);
        return parser.parseIntrusive(&_xmlText.front(), _xmlText.length(), SAXParser::ParseOption::HTML);
    }
    return true;
}

void RichText::initRenderer() {}

void RichText::insertElement(RichElement* element, int index)
{
    _richElements.insert(index, element);
    _formatTextDirty = true;
}

void RichText::pushBackElement(RichElement* element)
{
    _richElements.pushBack(element);
    _formatTextDirty = true;
}

void RichText::removeElement(int index)
{
    _richElements.erase(index);
    _formatTextDirty = true;
}

void RichText::removeElement(RichElement* element)
{
    _richElements.eraseObject(element);
    _formatTextDirty = true;
}

RichText::WrapMode RichText::getWrapMode() const
{
    return static_cast<RichText::WrapMode>(_defaults.at(KEY_WRAP_MODE).asInt());
}

void RichText::setWrapMode(RichText::WrapMode wrapMode)
{
    if (static_cast<RichText::WrapMode>(_defaults.at(KEY_WRAP_MODE).asInt()) != wrapMode)
    {
        _defaults[KEY_WRAP_MODE] = static_cast<int>(wrapMode);
        _formatTextDirty         = true;
    }
}

RichText::HorizontalAlignment RichText::getHorizontalAlignment() const
{
    return static_cast<RichText::HorizontalAlignment>(_defaults.at(KEY_HORIZONTAL_ALIGNMENT).asInt());
}

void RichText::setHorizontalAlignment(ax::ui::RichText::HorizontalAlignment a)
{
    if (static_cast<RichText::HorizontalAlignment>(_defaults.at(KEY_HORIZONTAL_ALIGNMENT).asInt()) != a)
    {
        _defaults[KEY_HORIZONTAL_ALIGNMENT] = static_cast<int>(a);
        _formatTextDirty                    = true;
    }
}

RichText::VerticalAlignment RichText::getVerticalAlignment() const
{
    return static_cast<RichText::VerticalAlignment>(_defaults.at(KEY_VERTICAL_ALIGNMENT).asInt());
}

void RichText::setVerticalAlignment(ax::ui::RichText::VerticalAlignment a)
{
    if (static_cast<RichText::VerticalAlignment>(_defaults.at(KEY_VERTICAL_ALIGNMENT).asInt()) != a)
    {
        _defaults[KEY_VERTICAL_ALIGNMENT] = static_cast<int>(a);
        _formatTextDirty                  = true;
    }
}

void RichText::setFontColor(std::string_view color)
{
    _defaults[KEY_FONT_COLOR_STRING] = color;
}

std::string RichText::getFontColor()
{
    return _defaults.at(KEY_FONT_COLOR_STRING).asString();
}

ax::Color32 RichText::getFontColor32()
{
    return parseColor32(getFontColor());
}

void RichText::setFontSize(float size)
{
    _defaults[KEY_FONT_SIZE] = size;
}

float RichText::getFontSize()
{
    return _defaults.at(KEY_FONT_SIZE).asFloat();
}

void RichText::setFontFace(std::string_view face)
{
    _defaults[KEY_FONT_FACE] = face;
}

std::string RichText::getFontFace()
{
    return _defaults.at(KEY_FONT_FACE).asString();
}

void RichText::setAnchorFontColor(std::string_view color)
{
    _defaults[KEY_ANCHOR_FONT_COLOR_STRING] = color;
}

std::string RichText::getAnchorFontColor()
{
    return _defaults.at(KEY_ANCHOR_FONT_COLOR_STRING).asString();
}

ax::Color32 RichText::getAnchorFontColor32()
{
    return parseColor32(getAnchorFontColor());
}

void RichText::setAnchorTextBold(bool enable)
{
    _defaults[KEY_ANCHOR_TEXT_BOLD] = enable;
}

bool RichText::isAnchorTextBoldEnabled()
{
    return _defaults[KEY_ANCHOR_TEXT_BOLD].asBool();
}

void RichText::setAnchorTextItalic(bool enable)
{
    _defaults[KEY_ANCHOR_TEXT_ITALIC] = enable;
}

bool RichText::isAnchorTextItalicEnabled()
{
    return _defaults[KEY_ANCHOR_TEXT_ITALIC].asBool();
}

void RichText::setAnchorTextDel(bool enable)
{
    if (enable)
        _defaults[KEY_ANCHOR_TEXT_LINE] = VALUE_TEXT_LINE_DEL;
    else if (_defaults[KEY_ANCHOR_TEXT_LINE].asString() == VALUE_TEXT_LINE_DEL)
        _defaults[KEY_ANCHOR_TEXT_LINE] = VALUE_TEXT_LINE_NONE;
}

bool RichText::isAnchorTextDelEnabled()
{
    return (_defaults[KEY_ANCHOR_TEXT_LINE].asString() == VALUE_TEXT_LINE_DEL);
}

void RichText::setAnchorTextUnderline(bool enable)
{
    if (enable)
        _defaults[KEY_ANCHOR_TEXT_LINE] = VALUE_TEXT_LINE_UNDER;
    else if (_defaults[KEY_ANCHOR_TEXT_LINE].asString() == VALUE_TEXT_LINE_UNDER)
        _defaults[KEY_ANCHOR_TEXT_LINE] = VALUE_TEXT_LINE_NONE;
}

bool RichText::isAnchorTextUnderlineEnabled()
{
    return (_defaults[KEY_ANCHOR_TEXT_LINE].asString() == VALUE_TEXT_LINE_UNDER);
}

void RichText::setAnchorTextOutline(bool enable, const Color32& outlineColor, int outlineSize)
{
    if (enable)
        _defaults[KEY_ANCHOR_TEXT_STYLE] = VALUE_TEXT_STYLE_OUTLINE;
    else if (_defaults[KEY_ANCHOR_TEXT_STYLE].asString() == VALUE_TEXT_STYLE_OUTLINE)
        _defaults[KEY_ANCHOR_TEXT_STYLE] = VALUE_TEXT_STYLE_NONE;
    _defaults[KEY_ANCHOR_TEXT_OUTLINE_COLOR] = formatColor32(outlineColor);
    _defaults[KEY_ANCHOR_TEXT_OUTLINE_SIZE]  = outlineSize;
}

bool RichText::isAnchorTextOutlineEnabled()
{
    return (_defaults[KEY_ANCHOR_TEXT_STYLE].asString() == VALUE_TEXT_STYLE_OUTLINE);
}

Color32 RichText::getAnchorTextOutlineColor32()
{
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_OUTLINE_COLOR); itr != _defaults.end())
    {
        return parseColor32(itr->second.asString());
    }
    return {};
}

int RichText::getAnchorTextOutlineSize()
{
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_OUTLINE_SIZE); itr != _defaults.end())
    {
        return itr->second.asInt();
    }
    return -1;
}

void RichText::setAnchorTextShadow(bool enable, const Color32& shadowColor, const Vec2& offset, int blurRadius)
{
    if (enable)
        _defaults[KEY_ANCHOR_TEXT_STYLE] = VALUE_TEXT_STYLE_SHADOW;
    else if (_defaults[KEY_ANCHOR_TEXT_STYLE].asString() == VALUE_TEXT_STYLE_SHADOW)
        _defaults[KEY_ANCHOR_TEXT_STYLE] = VALUE_TEXT_STYLE_NONE;
    _defaults[KEY_ANCHOR_TEXT_SHADOW_COLOR]         = formatColor32(shadowColor);
    _defaults[KEY_ANCHOR_TEXT_SHADOW_OFFSET_WIDTH]  = offset.width;
    _defaults[KEY_ANCHOR_TEXT_SHADOW_OFFSET_HEIGHT] = offset.height;
    _defaults[KEY_ANCHOR_TEXT_SHADOW_BLUR_RADIUS]   = blurRadius;
}

bool RichText::isAnchorTextShadowEnabled()
{
    return (_defaults[KEY_ANCHOR_TEXT_STYLE].asString() == VALUE_TEXT_STYLE_SHADOW);
}

Color32 RichText::getAnchorTextShadowColor32()
{
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_SHADOW_COLOR); itr != _defaults.end())
    {
        return parseColor32(itr->second.asString());
    }
    return {};
}

Vec2 RichText::getAnchorTextShadowOffset()
{
    float width  = 2.0f;
    float height = -2.0f;
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_SHADOW_OFFSET_WIDTH); itr != _defaults.end())
    {
        width = itr->second.asFloat();
    }
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_SHADOW_OFFSET_HEIGHT); itr != _defaults.end())
    {
        height = itr->second.asFloat();
    }
    return { width, height };
}

int RichText::getAnchorTextShadowBlurRadius()
{
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_SHADOW_BLUR_RADIUS); itr != _defaults.end())
    {
        return itr->second.asInt();
    }
    return 0;
}

void RichText::setAnchorTextGlow(bool enable, const Color32& glowColor)
{
    if (enable)
    {
        _defaults[KEY_ANCHOR_TEXT_STYLE] = VALUE_TEXT_STYLE_GLOW;
    }
    else if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_STYLE);
             itr != _defaults.end() && itr->second.asString() == VALUE_TEXT_STYLE_GLOW)
    {
        itr->second = VALUE_TEXT_STYLE_NONE;
    }
    _defaults[KEY_ANCHOR_TEXT_GLOW_COLOR] = formatColor32(glowColor);
}

bool RichText::isAnchorTextGlowEnabled()
{
    return (_defaults[KEY_ANCHOR_TEXT_STYLE].asString() == VALUE_TEXT_STYLE_GLOW);
}

Color32 RichText::getAnchorTextGlowColor32()
{
    if (auto&& itr = _defaults.find(KEY_ANCHOR_TEXT_GLOW_COLOR); itr != _defaults.end())
    {
        return parseColor32(itr->second.asString());
    }
    return {};
}

void RichText::setDefaults(const ValueMap& defaults)
{
    const auto setDefaultString = [this, defaults](const std::string& key) -> void {
        if (auto&& itr = defaults.find(key); itr != defaults.end())
        {
            _defaults[key] = itr->second.asString();
        }
    };
    const auto setDefaultInt = [this, defaults](const std::string& key) -> void {
        if (auto&& itr = defaults.find(key); itr != defaults.end())
        {
            _defaults[key] = itr->second.asInt();
        }
    };
    const auto setDefaultFloat = [this, defaults](const std::string& key) -> void {
        if (auto&& itr = defaults.find(key); itr != defaults.end())
        {
            _defaults[key] = itr->second.asFloat();
        }
    };
    const auto setDefaultBool = [this, defaults](const std::string& key) -> void {
        if (auto&& itr = defaults.find(key); itr != defaults.end())
        {
            _defaults[key] = itr->second.asBool();
        }
    };

    setDefaultFloat(KEY_VERTICAL_SPACE);
    setDefaultInt(KEY_WRAP_MODE);
    setDefaultInt(KEY_HORIZONTAL_ALIGNMENT);
    setDefaultInt(KEY_VERTICAL_ALIGNMENT);
    setDefaultString(KEY_FONT_COLOR_STRING);
    setDefaultFloat(KEY_FONT_SIZE);
    setDefaultString(KEY_FONT_FACE);
    setDefaultString(KEY_ANCHOR_FONT_COLOR_STRING);
    setDefaultBool(KEY_ANCHOR_TEXT_BOLD);
    setDefaultBool(KEY_ANCHOR_TEXT_ITALIC);
    setDefaultString(KEY_ANCHOR_TEXT_LINE);
    setDefaultString(KEY_ANCHOR_TEXT_STYLE);
    setDefaultString(KEY_ANCHOR_TEXT_OUTLINE_COLOR);
    setDefaultInt(KEY_ANCHOR_TEXT_OUTLINE_SIZE);
    setDefaultString(KEY_ANCHOR_TEXT_SHADOW_COLOR);
    setDefaultFloat(KEY_ANCHOR_TEXT_SHADOW_OFFSET_WIDTH);
    setDefaultFloat(KEY_ANCHOR_TEXT_SHADOW_OFFSET_HEIGHT);
    setDefaultInt(KEY_ANCHOR_TEXT_SHADOW_BLUR_RADIUS);
    setDefaultString(KEY_ANCHOR_TEXT_GLOW_COLOR);
}

ValueMap RichText::getDefaults() const
{
    ValueMap defaults;
    return defaults;
}

static inline bool parseHexDigit(uint8_t& out, const char* p, int n)
{
    auto ret = axstd::from_chars(p, p + n, out, 16);
    return ret.ec == std::errc{};
}

Color32 RichText::parseColor32(std::string_view color)
{
    Color32 ret = Color32::WHITE;

    if (!color.empty() && color[0] == '#') [[likely]]
    {
        auto ptr = color.data();
        switch (color.length())
        {
        case 4:
            if (parseHexDigit(ret.r, ptr + 1, 1))
                ret.r += ret.r * 16;
            if (parseHexDigit(ret.g, ptr + 2, 1))
                ret.g += ret.g * 16;
            if (parseHexDigit(ret.b, ptr + 3, 1))
                ret.b += ret.b * 16;
            break;
        case 5:
            if (parseHexDigit(ret.r, ptr + 1, 1))
                ret.r += ret.r * 16;
            if (parseHexDigit(ret.g, ptr + 2, 1))
                ret.g += ret.g * 16;
            if (parseHexDigit(ret.b, ptr + 3, 1))
                ret.b += ret.b * 16;
            if (parseHexDigit(ret.a, ptr + 4, 1))
                ret.a += ret.a * 16;
            break;
        case 7:
            parseHexDigit(ret.r, ptr + 1, 2);
            parseHexDigit(ret.g, ptr + 3, 2);
            parseHexDigit(ret.b, ptr + 5, 2);
            break;
        case 9:
            parseHexDigit(ret.r, ptr + 1, 2);
            parseHexDigit(ret.g, ptr + 3, 2);
            parseHexDigit(ret.b, ptr + 5, 2);
            parseHexDigit(ret.a, ptr + 7, 2);
            break;
        }
    }

    return ret; // default color if parsing fails
}

std::string RichText::formatColor32(const ax::Color32& color)
{
    return fmt::format("#{:02x}{:02x}{:02x}{:02x}", color.r, color.g, color.b, color.a);
}

void RichText::setTagDescription(std::string_view tag,
                                 bool isFontElement,
                                 VisitEnterHandler handleVisitEnter, VisitExitHandler handleVisitExit)
{
    MyXMLVisitor::setTagDescription(tag, isFontElement, std::move(handleVisitEnter), std::move(handleVisitExit));
}

void RichText::removeTagDescription(std::string_view tag)
{
    MyXMLVisitor::removeTagDescription(tag);
}

void RichText::openUrl(std::string_view url)
{
    if (_handleOpenUrl)
    {
        _handleOpenUrl(url);
    }
    else if (!url.empty())
    {
        Application::getInstance()->openURL(url);
    }
}

void RichText::setOpenUrlHandler(const OpenUrlHandler& handleOpenUrl)
{
    _handleOpenUrl = handleOpenUrl;
}

void RichText::formatText(bool force)
{
    _formatTextDirty |= force;
    if (_formatTextDirty)
    {
        this->removeAllProtectedChildren();
        _elementRenders.clear();
        _lineHeights.clear();
        if (_ignoreSize)
        {
            addNewLine();
            for (ssize_t i = 0, size = _richElements.size(); i < size; ++i)
            {
                RichElement* element  = _richElements.at(i);
                Node* elementRenderer = nullptr;
                switch (element->_type)
                {
                case RichElement::Type::TEXT:
                {
                    RichElementText* elmtText = static_cast<RichElementText*>(element);
                    Label* label;
                    if (FileUtils::getInstance()->isFileExist(elmtText->_fontName))
                    {
                        label = Label::createWithTTF(elmtText->_text, elmtText->_fontName, elmtText->_fontSize);
                    }
                    else
                    {
                        label = Label::createWithSystemFont(elmtText->_text, elmtText->_fontName, elmtText->_fontSize);
                    }
                    if (elmtText->_flags & RichElementText::ITALICS_FLAG)
                        label->enableItalics();
                    if (elmtText->_flags & RichElementText::BOLD_FLAG)
                        label->enableBold();
                    if (elmtText->_flags & RichElementText::UNDERLINE_FLAG)
                        label->enableUnderline();
                    if (elmtText->_flags & RichElementText::STRIKETHROUGH_FLAG)
                        label->enableStrikethrough();
                    if (elmtText->_flags & RichElementText::URL_FLAG)
                        label->addComponent(UrlTouchListenerComponent::create(
                            label, elmtText->_url, [this](std::string_view url) { openUrl(url); }));
                    if (elmtText->_flags & RichElementText::OUTLINE_FLAG)
                    {
                        label->enableOutline(Color32(elmtText->_outlineColor), elmtText->_outlineSize);
                    }
                    if (elmtText->_flags & RichElementText::SHADOW_FLAG)
                    {
                        label->enableShadow(Color32(elmtText->_shadowColor), elmtText->_shadowOffset,
                                            elmtText->_shadowBlurRadius);
                    }
                    if (elmtText->_flags & RichElementText::GLOW_FLAG)
                    {
                        label->enableGlow(Color32(elmtText->_glowColor));
                    }
                    label->setTextColor(Color32(elmtText->_color));

                    label->setName(elmtText->_id);

                    elementRenderer = label;
                    break;
                }
                case RichElement::Type::IMAGE:
                {
                    RichElementImage* elmtImage = static_cast<RichElementImage*>(element);
                    if (elmtImage->_textureType == Widget::TextureResType::LOCAL)
                        elementRenderer = Sprite::create(elmtImage->_filePath);
                    else
                        elementRenderer = Sprite::createWithSpriteFrameName(elmtImage->_filePath);

                    if (elementRenderer && (elmtImage->_height != -1 || elmtImage->_width != -1))
                    {
                        auto currentSize = elementRenderer->getContentSize();
                        if (elmtImage->_width != -1)
                            elementRenderer->setScaleX((elmtImage->_width / currentSize.width) * elmtImage->_scaleX);
                        else
                            elementRenderer->setScaleX(elmtImage->_scaleX);

                        if (elmtImage->_height != -1)
                            elementRenderer->setScaleY((elmtImage->_height / currentSize.height) * elmtImage->_scaleY);
                        else
                            elementRenderer->setScaleY(elmtImage->_scaleY);

                        elementRenderer->setContentSize(Vec2(currentSize.width * elementRenderer->getScaleX(),
                                                             currentSize.height * elementRenderer->getScaleY()));
                        elementRenderer->addComponent(
                            UrlTouchListenerComponent::create(elementRenderer, elmtImage->_url,
                                                      std::bind(&RichText::openUrl, this, std::placeholders::_1)));
                        elementRenderer->setColor(element->_color);
                        elementRenderer->setName(elmtImage->_id);
                    }
                    break;
                }
                case RichElement::Type::CUSTOM:
                {
                    RichElementCustomNode* elmtCustom = static_cast<RichElementCustomNode*>(element);
                    elementRenderer                   = elmtCustom->_customNode;
                    elementRenderer->setColor(element->_color);
                    break;
                }
                case RichElement::Type::NEWLINE:
                {
                    auto* newLineMulti = static_cast<RichElementNewLine*>(element);

                    addNewLine(newLineMulti->_quantity);
                    break;
                }
                default:
                    break;
                }

                if (elementRenderer)
                {
                    // not needed ??
                    // elementRenderer->setOpacity(element->_color.a);
                    pushToContainer(elementRenderer);
                }
            }
        }
        else
        {
            addNewLine();
            for (ssize_t i = 0, size = _richElements.size(); i < size; ++i)
            {
                RichElement* element = _richElements.at(i);
                switch (element->_type)
                {
                case RichElement::Type::TEXT:
                {
                    RichElementText* elmtText = static_cast<RichElementText*>(element);
                    handleTextRenderer(elmtText->_text, elmtText->_fontName, elmtText->_fontSize, elmtText->_color,
                                       elmtText->_flags, elmtText->_url, elmtText->_outlineColor,
                                       elmtText->_outlineSize, elmtText->_shadowColor, elmtText->_shadowOffset,
                                       elmtText->_shadowBlurRadius, elmtText->_glowColor, elmtText->_id);
                    break;
                }
                case RichElement::Type::IMAGE:
                {
                    RichElementImage* elmtImage = static_cast<RichElementImage*>(element);
                    handleImageRenderer(elmtImage->_filePath, elmtImage->_textureType, elmtImage->_color,
                                        elmtImage->_width, elmtImage->_height, elmtImage->_url,
                                        elmtImage->_scaleX, elmtImage->_scaleY, elmtImage->_id);
                    break;
                }
                case RichElement::Type::CUSTOM:
                {
                    RichElementCustomNode* elmtCustom = static_cast<RichElementCustomNode*>(element);
                    handleCustomRenderer(elmtCustom->_customNode, elmtCustom->_id);
                    break;
                }
                case RichElement::Type::NEWLINE:
                {
                    auto* newLineMulti = static_cast<RichElementNewLine*>(element);

                    addNewLine(newLineMulti->_quantity);
                    break;
                }
                default:
                    break;
                }
            }
        }
        formatRenderers();
        _formatTextDirty = false;
    }
}

namespace
{
inline bool isUTF8CharWrappable(const text_utils::StringUTF8::CharUTF8& ch)
{
    return (!ch.isASCII() || !std::isgraph(ch._char[0], std::locale()));
}

int getPrevWordPos(const text_utils::StringUTF8& text, int idx)
{
    if (idx <= 0)
        return -1;

    // start from idx-1
    const text_utils::StringUTF8::CharUTF8Store& str = text.getString();
    const auto it = std::find_if(str.rbegin() + (str.size() - idx + 1), str.rend(), isUTF8CharWrappable);
    if (it == str.rend())
        return -1;
    return static_cast<int>(it.base() - str.begin());
}

int getNextWordPos(const text_utils::StringUTF8& text, int idx)
{
    const text_utils::StringUTF8::CharUTF8Store& str = text.getString();
    if (idx + 1 >= static_cast<int>(str.size()))
        return static_cast<int>(str.size());

    const auto it = std::find_if(str.begin() + idx + 1, str.end(), isUTF8CharWrappable);
    return static_cast<int>(it - str.begin());
}

bool isWrappable(const text_utils::StringUTF8& text)
{
    const text_utils::StringUTF8::CharUTF8Store& str = text.getString();
    return std::any_of(str.begin(), str.end(), isUTF8CharWrappable);
}

int findSplitPositionForWord(Label* label,
                             const text_utils::StringUTF8& text,
                             int estimatedIdx,
                             float originalLeftSpaceWidth,
                             float newLineWidth)
{
    const bool startingNewLine = (newLineWidth == originalLeftSpaceWidth);
    if (!isWrappable(text))
        return (startingNewLine ? static_cast<int>(text.length()) : 0);

    // The adjustment of the new line position
    int idx             = getNextWordPos(text, estimatedIdx);
    std::string leftStr = text.getAsCharSequence(0, idx);
    label->setString(leftStr);
    float textRendererWidth = label->getContentSize().width;
    if (originalLeftSpaceWidth < textRendererWidth)  // Have protruding
    {
        while (1)
        {
            // try to erase a word
            int newidx = getPrevWordPos(text, idx);
            if (newidx >= 0)
            {
                leftStr = text.getAsCharSequence(0, newidx);
                label->setString(leftStr);
                textRendererWidth = label->getContentSize().width;
                if (textRendererWidth <= originalLeftSpaceWidth)  // is fitted
                    return newidx;
                idx = newidx;
                continue;
            }
            // newidx < 0 means no prev word
            return (startingNewLine ? idx : 0);
        }
    }
    else if (textRendererWidth < originalLeftSpaceWidth)  // A wide margin
    {
        while (1)
        {
            // try to append a word
            int newidx = getNextWordPos(text, idx);
            leftStr    = text.getAsCharSequence(0, newidx);
            label->setString(leftStr);
            textRendererWidth = label->getContentSize().width;
            if (textRendererWidth < originalLeftSpaceWidth)
            {
                // the whole string is tested
                if (newidx == static_cast<int>(text.length()))
                    return newidx;
                idx = newidx;
                continue;
            }
            // protruded ? undo add, or quite fit
            return (textRendererWidth > originalLeftSpaceWidth ? idx : newidx);
        }
    }

    return idx;
}

int findSplitPositionForChar(Label* label,
                             const text_utils::StringUTF8& text,
                             int estimatedIdx,
                             float originalLeftSpaceWidth,
                             float newLineWidth)
{
    bool startingNewLine = (newLineWidth == originalLeftSpaceWidth);

    int stringLength = static_cast<int>(text.length());
    int leftLength   = estimatedIdx;

    // The adjustment of the new line position
    std::string leftStr = text.getAsCharSequence(0, leftLength);
    label->setString(leftStr);
    float textRendererWidth = label->getContentSize().width;
    if (originalLeftSpaceWidth < textRendererWidth)  // Have protruding
    {
        while (leftLength-- > 0)
        {
            // try to erase a char
            auto& ch = text.getString().at(leftLength);
            leftStr.erase(leftStr.end() - ch._char.length(), leftStr.end());
            label->setString(leftStr);
            textRendererWidth = label->getContentSize().width;
            if (textRendererWidth <= originalLeftSpaceWidth)  // is fitted
                break;
        }
    }
    else if (textRendererWidth < originalLeftSpaceWidth)  // A wide margin
    {
        while (leftLength < stringLength)
        {
            // try to append a char
            auto& ch = text.getString().at(leftLength);
            ++leftLength;
            leftStr.append(ch._char);
            label->setString(leftStr);
            textRendererWidth = label->getContentSize().width;
            if (originalLeftSpaceWidth < textRendererWidth)  // protruded, undo add
            {
                --leftLength;
                break;
            }
            else if (originalLeftSpaceWidth == textRendererWidth)  // quite fit
            {
                break;
            }
        }
    }

    if (leftLength <= 0)
        return (startingNewLine) ? 1 : 0;
    return leftLength;
}
}  // namespace

void RichText::handleTextRenderer(std::string_view text,
                                  std::string_view fontName,
                                  float fontSize,
                                  const Color32& color,
                                  uint32_t flags,
                                  std::string_view url,
                                  const Color32& outlineColor,
                                  int outlineSize,
                                  const Color32& shadowColor,
                                  const Vec2& shadowOffset,
                                  int shadowBlurRadius,
                                  const Color32& glowColor,
                                  std::string_view id)
{
    bool fileExist              = FileUtils::getInstance()->isFileExist(fontName);
    RichText::WrapMode wrapMode = static_cast<RichText::WrapMode>(_defaults.at(KEY_WRAP_MODE).asInt());

    // split text by \n
    std::stringstream ss;
    ss << text;
    std::string currentText;
    size_t realLines = 0;
    auto isFirstLabel = true;
    while (std::getline(ss, currentText, '\n'))
    {
        if (realLines > 0)
        {
            addNewLine();
            _lineHeights.back() = fontSize;
        }
        ++realLines;

        size_t splitParts = 0;
        text_utils::StringUTF8 utf8Text(currentText);
        while (!currentText.empty())
        {
            if (splitParts > 0)
            {
                addNewLine();
                _lineHeights.back() = fontSize;
            }
            ++splitParts;

            Label* textRenderer = fileExist ? Label::createWithTTF(currentText, fontName, fontSize)
                                            : Label::createWithSystemFont(currentText, fontName, fontSize);

            if (flags & RichElementText::ITALICS_FLAG)
                textRenderer->enableItalics();
            if (flags & RichElementText::BOLD_FLAG)
                textRenderer->enableBold();
            if (flags & RichElementText::UNDERLINE_FLAG)
                textRenderer->enableUnderline();
            if (flags & RichElementText::STRIKETHROUGH_FLAG)
                textRenderer->enableStrikethrough();
            if (flags & RichElementText::URL_FLAG)
                textRenderer->addComponent(UrlTouchListenerComponent::create(
                    textRenderer, url, [this](std::string_view url) { openUrl(url); }));
            if (flags & RichElementText::OUTLINE_FLAG)
                textRenderer->enableOutline(Color32(outlineColor), outlineSize);
            if (flags & RichElementText::SHADOW_FLAG)
                textRenderer->enableShadow(Color32(shadowColor), shadowOffset, shadowBlurRadius);
            if (flags & RichElementText::GLOW_FLAG)
                textRenderer->enableGlow(Color32(glowColor));

            textRenderer->setTextColor(color);

            if (isFirstLabel && !id.empty())
            {
                textRenderer->setName(id);
                isFirstLabel = false;
            }

            // textRendererWidth will get 0.0f, when we've got glError: 0x0501 in Label::getContentSize
            // It happens when currentText is very very long so that can't generate a texture
            const float textRendererWidth = textRenderer->getContentSize().width;

            // no splitting
            if (textRendererWidth > 0.0f && _leftSpaceWidth >= textRendererWidth)
            {
                _leftSpaceWidth -= textRendererWidth;
                pushToContainer(textRenderer);
                break;
            }

            // rough estimate
            // when textRendererWidth == 0.0f, use fontSize as the rough estimate of width for each char,
            //  (_leftSpaceWidth / fontSize) means how many chars can be aligned in leftSpaceWidth.
            int estimatedIdx = 0;
            if (textRendererWidth > 0.0f)
                estimatedIdx = static_cast<int>(_leftSpaceWidth / textRendererWidth * utf8Text.length());
            else
                estimatedIdx = static_cast<int>(_leftSpaceWidth / fontSize);

            int leftLength = 0;
            if (wrapMode == WRAP_PER_WORD)
                leftLength =
                    findSplitPositionForWord(textRenderer, utf8Text, estimatedIdx, _leftSpaceWidth, _customSize.width);
            else
                leftLength =
                    findSplitPositionForChar(textRenderer, utf8Text, estimatedIdx, _leftSpaceWidth, _customSize.width);

            // split string
            if (leftLength > 0)
            {
                textRenderer->setString(utf8Text.getAsCharSequence(0, leftLength));
                pushToContainer(textRenderer);
            }

            text_utils::StringUTF8::CharUTF8Store& str = utf8Text.getString();

            // after the first line, skip any spaces to the left
            const auto startOfWordItr = std::find_if(
                str.begin() + leftLength, str.end(),
                [](const text_utils::StringUTF8::CharUTF8& ch) { return !std::isspace(ch._char[0], std::locale()); });
            if (startOfWordItr != str.end())
                leftLength = static_cast<int>(startOfWordItr - str.begin());

            // erase the chars which are processed
            str.erase(str.begin(), str.begin() + leftLength);
            currentText = utf8Text.getAsCharSequence();
        }
    }

    // std::getline discards the delimiter, so if it exists at the end of the text, then
    // a new line entry should be added
    if (!text.empty() && (text.back() == '\n'))
    {
        addNewLine();
        _lineHeights.back() = fontSize;
    }
}

void RichText::handleImageRenderer(std::string_view filePath,
                                   Widget::TextureResType textureType,
                                   const Color32& /*color*/,
                                   int width,
                                   int height,
                                   std::string_view url,
                                   float scaleX,
                                   float scaleY,
                                   std::string_view id)
{
    Sprite* imageRenderer;
    if (textureType == Widget::TextureResType::LOCAL)
        imageRenderer = Sprite::create(filePath);
    else
        imageRenderer = Sprite::createWithSpriteFrameName(filePath);

    if (imageRenderer)
    {
        auto currentSize = imageRenderer->getContentSize();
        if (width != -1)
            imageRenderer->setScaleX(width / currentSize.width);
        if (height != -1)
            imageRenderer->setScaleY(height / currentSize.height);

        imageRenderer->setName(id);

        imageRenderer->setScaleX(imageRenderer->getScaleX() * scaleX);
        imageRenderer->setScaleY(imageRenderer->getScaleY() * scaleY);

        imageRenderer->setContentSize(
            Vec2(currentSize.width * imageRenderer->getScaleX(), currentSize.height * imageRenderer->getScaleY()));
        imageRenderer->setScale(1.f, 1.f);
        handleCustomRenderer(imageRenderer);
        imageRenderer->addComponent(
            UrlTouchListenerComponent::create(imageRenderer, url, [this](std::string_view url) { openUrl(url); }));
    }
}

void RichText::handleCustomRenderer(ax::Node* renderer, std::string_view id)
{
    Vec2 imgSize = renderer->getContentSize();

    if (!id.empty())
    {
        renderer->setName(id);
    }

    _leftSpaceWidth -= imgSize.width;
    if (_leftSpaceWidth < 0.0f)
    {
        addNewLine();
        pushToContainer(renderer);
        _leftSpaceWidth -= imgSize.width;
    }
    else
    {
        pushToContainer(renderer);
    }
}

void RichText::addNewLine(int quantity)
{
    do
    {
        _leftSpaceWidth = _customSize.width;
        _elementRenders.emplace_back();
        _lineHeights.emplace_back();
    }
    while (--quantity > 0);
}

void RichText::formatRenderers()
{
    float verticalSpace = _defaults[KEY_VERTICAL_SPACE].asFloat();
    float fontSize      = _defaults[KEY_FONT_SIZE].asFloat();

    if (_ignoreSize)
    {
        const auto verticalAlignment = static_cast<VerticalAlignment>(_defaults.at(KEY_VERTICAL_ALIGNMENT).asInt());

        float newContentSizeWidth = 0.0f;
        float nextPosY            = 0.0f;
        std::vector<std::pair<Vector<Node*>*, float>> rowWidthPairs;
        rowWidthPairs.reserve(_elementRenders.size());
        for (auto&& element : _elementRenders)
        {
            float nextPosX = 0.0f;
            float maxY     = 0.0f;
            for (auto&& iter : element)
            {
                auto& iSize = iter->getContentSize();

                if (verticalAlignment == VerticalAlignment::CENTER)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                    iter->setPosition(nextPosX, nextPosY + iSize.height / 2);
                }
                else if (verticalAlignment == VerticalAlignment::TOP)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
                    iter->setPosition(nextPosX, nextPosY + iSize.height);
                }
                else  // if (verticalAlignment == VerticalAlignment::BOTTOM)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
                    iter->setPosition(nextPosX, nextPosY);
                }

                this->addProtectedChild(iter, 1);
                newContentSizeWidth += iSize.width;
                nextPosX += iSize.width;
                maxY = std::max(maxY, iSize.height);
            }
            nextPosY -= maxY;
            rowWidthPairs.emplace_back(&element, nextPosX);
        }
        this->setContentSize(Vec2(newContentSizeWidth, -nextPosY));
        for (auto&& row : rowWidthPairs)
            doHorizontalAlignment(*row.first, row.second);
    }
    else
    {
        // calculate real height
        float newContentSizeHeight = 0.0f;
        std::vector<float> maxHeights(_elementRenders.size());

        for (size_t i = 0, size = _elementRenders.size(); i < size; i++)
        {
            Vector<Node*>& row = _elementRenders[i];
            float maxHeight    = 0.0f;
            for (auto&& iter : row)
            {
                maxHeight = std::max(iter->getContentSize().height, maxHeight);
            }

            // gap for empty line, if _lineHeights[i] == 0, use current RichText's fontSize
            if (row.empty())
            {
                maxHeight = (_lineHeights[i] != 0.0f ? _lineHeights[i] : fontSize);
            }
            maxHeights[i] = maxHeight;

            // vertical space except for first line
            newContentSizeHeight += (i != 0 ? maxHeight + verticalSpace : maxHeight);
        }
        _customSize.height = newContentSizeHeight;

        const auto verticalAlignment = static_cast<VerticalAlignment>(_defaults.at(KEY_VERTICAL_ALIGNMENT).asInt());

        // align renders
        float nextPosY = _customSize.height;
        for (size_t i = 0, size = _elementRenders.size(); i < size; i++)
        {
            Vector<Node*>& row    = _elementRenders[i];
            float nextPosX        = 0.0f;
            const auto lineHeight = maxHeights[i];
            nextPosY -= (i != 0 ? lineHeight + verticalSpace : lineHeight);
            for (auto&& iter : row)
            {
                if (verticalAlignment == VerticalAlignment::CENTER)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_MIDDLE_LEFT);
                    iter->setPosition(nextPosX, nextPosY + lineHeight / 2);
                }
                else if (verticalAlignment == VerticalAlignment::TOP)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_TOP_LEFT);
                    iter->setPosition(nextPosX, nextPosY + lineHeight);
                }
                else // if (verticalAlignment == VerticalAlignment::BOTTOM)
                {
                    iter->setAnchorPoint(Vec2::ANCHOR_BOTTOM_LEFT);
                    iter->setPosition(nextPosX, nextPosY);
                }
                this->addProtectedChild(iter, 1);
                nextPosX += iter->getContentSize().width;
            }

            doHorizontalAlignment(row, nextPosX);
        }
    }

    _elementRenders.clear();
    _lineHeights.clear();

    if (_ignoreSize)
    {
        Vec2 s = getVirtualRendererSize();
        this->setContentSize(s);
    }
    else
    {
        this->setContentSize(_customSize);
    }
    updateContentSizeWithTextureSize(_contentSize);
}

namespace
{
float getPaddingAmount(const RichText::HorizontalAlignment alignment, const float leftOver)
{
    switch (alignment)
    {
    case RichText::HorizontalAlignment::CENTER:
        return leftOver / 2.f;
    case RichText::HorizontalAlignment::RIGHT:
        return leftOver;
    default:
        AXASSERT(false, "invalid horizontal alignment!");
        return 0.f;
    }
}
}  // namespace

void RichText::doHorizontalAlignment(const Vector<ax::Node*>& row, float rowWidth)
{
    const auto alignment = static_cast<HorizontalAlignment>(_defaults.at(KEY_HORIZONTAL_ALIGNMENT).asInt());
    if (alignment != HorizontalAlignment::LEFT)
    {
        const auto diff         = stripTrailingWhitespace(row);
        const auto leftOver     = getContentSize().width - (rowWidth + diff);
        const float leftPadding = getPaddingAmount(alignment, leftOver);
        const Vec2 offset(leftPadding, 0.f);
        for (auto&& node : row)
        {
            node->setPosition(node->getPosition() + offset);
        }
    }
}

namespace
{
bool isWhitespace(char c)
{
    return std::isspace(c, std::locale());
}
void rtrim(std::string& s)
{
    s.erase(std::find_if_not(s.rbegin(), s.rend(), isWhitespace).base(), s.end());
}
}  // namespace

float RichText::stripTrailingWhitespace(const Vector<ax::Node*>& row)
{
    if (!row.empty())
    {
        if (auto label = dynamic_cast<Label*>(row.back()))
        {
            const auto width = label->getContentSize().width;
            std::string trimmedString{label->getString()};
            rtrim(trimmedString);
            if (label->getString() != trimmedString)
            {
                label->setString(trimmedString);
                return label->getContentSize().width - width;
            }
        }
    }
    return 0.0f;
}

void RichText::adaptRenderers()
{
    this->formatText();
}

void RichText::pushToContainer(ax::Node* renderer)
{
    if (_elementRenders.empty())
    {
        return;
    }
    _elementRenders[_elementRenders.size() - 1].pushBack(renderer);
}

void RichText::setVerticalSpace(float space)
{
    _defaults[KEY_VERTICAL_SPACE] = space;
}

void RichText::ignoreContentAdaptWithSize(bool ignore)
{
    if (_ignoreSize != ignore)
    {
        _formatTextDirty = true;
        Widget::ignoreContentAdaptWithSize(ignore);
    }
}

std::string RichText::getDescription() const
{
    return "RichText";
}
