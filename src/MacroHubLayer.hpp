#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/utils/Task.hpp>
#include <Geode/loader/Event.hpp>

using namespace geode::prelude;

class MacroHubLayer : public cocos2d::CCLayerColor {
protected:
    ScrollLayer* m_scroll = nullptr;
    TextInput* m_searchInput = nullptr;
    matjson::Value m_fullData;
    std::string m_tempDownloadData;

    // Explicit listener for file picking tasks
    geode::EventListener<geode::Task<geode::utils::file::PickResult>> m_pickListener;

    bool init() override;
    void onUpload(CCObject*);
    void onRefresh(CCObject*);
    void onDownload(CCObject* sender);
    void onSearch(CCObject*);
    
    void loadList(matjson::Value data);
    bool canUpload();
    void incrementUploadCount();
    void onClose(CCObject*);

public:
    static MacroHubLayer* create();
    void show();
};