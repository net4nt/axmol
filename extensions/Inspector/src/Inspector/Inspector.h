#pragma once

#include <memory>
#include "extensions/ExtensionMacros.h"
#include "axmol/base/Config.h"
#include <string>
#include <unordered_map>
#include "axmol/base/EventListenerCustom.h"
#include "axmol/base/RefPtr.h"
#include "axmol/2d/Node.h"

namespace ax
{
class Node;
class Scene;

}

NS_AX_EXT_BEGIN

class InspectPropertyHandler
{
public:
    virtual ~InspectPropertyHandler() = default;
    virtual bool isSupportedType(Node* node) = 0;
    virtual void drawProperties(Node* node) = 0;
};

class InspectorNodePropertyHandler : public InspectPropertyHandler
{
public:
    ~InspectorNodePropertyHandler() override = default;
    bool isSupportedType(Node* node) override;
    void drawProperties(Node* node) override;
};

class InspectorSpritePropertyHandler : public InspectPropertyHandler
{
public:
    ~InspectorSpritePropertyHandler() override = default;
    bool isSupportedType(Node* node) override;
    void drawProperties(Node* node) override;
};

class InspectorLabelProtocolPropertyHandler : public InspectPropertyHandler
{
public:
    ~InspectorLabelProtocolPropertyHandler() override = default;
    bool isSupportedType(Node* node) override;
    void drawProperties(Node* node) override;
};

class Inspector
{
  public:
    static Inspector* getInstance();
    static void destroyInstance();
    static std::string getNodeTypeName(Node*);
    static std::string demangle(const char* mangled);
    void openForScene(Scene*);
    void openForCurrentScene();
    void close();

    bool addPropertyHandler(std::string_view handlerId, std::unique_ptr<InspectPropertyHandler> handler);
    void removePropertyHandler(const std::string& handlerId);

    void setAutoAddToScenes(bool autoAdd);

    std::string_view getFontPath() const { return _fontPath; }
    float getFontSize() const { return _fontSize; }
    void setFontPath(std::string_view fontPath);
    void setFontSize(float fontSize);

  private:
    void init();
    void cleanup();
    void mainLoop();
    void drawTreeRecursive(Node*, int index = 0);
    void drawProperties();

    ax::RefPtr<ax::Node> _selected_node = nullptr;
    ax::Scene* _target = nullptr;

    std::unordered_map<std::string, std::unique_ptr<InspectPropertyHandler>> _propertyHandlers;
    RefPtr<EventListenerCustom> _beforeNewSceneEventListener;
    RefPtr<EventListenerCustom> _afterNewSceneEventListener;

    bool _autoAddToScenes = false;
    std::string _fontPath;
    float _fontSize;
};

NS_AX_EXT_END
