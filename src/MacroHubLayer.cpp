#include "MacroHubLayer.hpp"
#include "MacroManager.hpp"
#include <algorithm>
#include <chrono>

// Helper function for the daily limit date
std::string getTodayDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    struct tm* timeinfo = std::localtime(&now_time);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeinfo);
    return std::string(buffer);
}

MacroHubLayer* MacroHubLayer::create() {
    auto ret = new MacroHubLayer();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool MacroHubLayer::init() {
    if (!CCLayerColor::initWithColor({0, 0, 0, 150})) return false;

    auto winSize = CCDirector::sharedDirector()->getWinSize();

    auto bg = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    bg->setContentSize({360.f, 260.f});
    bg->setPosition(winSize.width / 2, winSize.height / 2);
    this->addChild(bg);

    // Title
    auto title = CCLabelBMFont::create("Macro Hub", "bigFont.fnt");
    title->setPosition(winSize.width / 2, winSize.height / 2 + 110.f);
    title->setScale(0.6f);
    this->addChild(title);

    // Search Input
    m_searchInput = TextInput::create(180.f, "Search...", "chatFont.fnt");
    m_searchInput->setPosition(winSize.width / 2 - 30.f, winSize.height / 2 + 80.f);
    m_searchInput->setScale(0.75f);
    this->addChild(m_searchInput);

    auto menu = CCMenu::create();
    menu->setPosition(0, 0);
    this->addChild(menu);

    // Search Button
    auto searchBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("gj_findBtn_001.png"),
        this, menu_selector(MacroHubLayer::onSearch)
    );
    searchBtn->setPosition(winSize.width / 2 + 75.f, winSize.height / 2 + 80.f);
    searchBtn->setScale(0.7f);
    menu->addChild(searchBtn);

    // Close Button
    auto closeBtn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png"),
        this, menu_selector(MacroHubLayer::onClose)
    );
    closeBtn->setPosition(winSize.width / 2 - 170.f, winSize.height / 2 + 120.f);
    menu->addChild(closeBtn);

    // Upload Button
    auto uploadBtn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Upload Macro", "goldFont.fnt", "GJ_button_01.png", 0.7f),
        this, menu_selector(MacroHubLayer::onUpload)
    );
    uploadBtn->setPosition(winSize.width / 2, winSize.height / 2 - 110.f);
    menu->addChild(uploadBtn);

    this->setTouchEnabled(true);
    this->setKeypadEnabled(true);

    onRefresh(nullptr);

    return true;
}

void MacroHubLayer::onSearch(CCObject*) {
    std::string query = m_searchInput->getString();
    if (query.empty() || !m_fullData.isArray()) {
        loadList(m_fullData);
        return;
    }

    // Fixed matjson error by using a std::vector first
    std::vector<matjson::Value> filteredVec;
    auto arr = m_fullData.asArray().unwrap();
    for (auto& item : arr) {
        std::string name = item["name"].asString().unwrapOr("");
        
        // Basic case-insensitive search
        std::string lowName = name;
        std::string lowQuery = query;
        std::transform(lowName.begin(), lowName.end(), lowName.begin(), ::tolower);
        std::transform(lowQuery.begin(), lowQuery.end(), lowQuery.begin(), ::tolower);

        if (lowName.find(lowQuery) != std::string::npos) {
            filteredVec.push_back(item);
        }
    }
    loadList(matjson::Value(filteredVec));
}

void MacroHubLayer::onRefresh(CCObject*) {
    if (m_scroll) m_scroll->removeFromParent();
    
    MacroManager::fetchMacros([this](matjson::Value data) {
        m_fullData = data;
        this->loadList(data);
    });
}

void MacroHubLayer::loadList(matjson::Value data) {
    if (m_scroll) m_scroll->removeFromParent();
    auto winSize = CCDirector::sharedDirector()->getWinSize();

    m_scroll = ScrollLayer::create({310.f, 135.f});
    m_scroll->setPosition(winSize.width / 2 - 155.f, winSize.height / 2 - 55.f);
    this->addChild(m_scroll);

    if (!data.isArray()) return;
    auto arr = data.asArray().unwrap();

    float itemHeight = 35.f;
    float totalHeight = std::max(135.f, (float)arr.size() * itemHeight);
    m_scroll->m_contentLayer->setContentSize({310.f, totalHeight});

    auto listMenu = CCMenu::create();
    listMenu->setPosition(0, 0);
    m_scroll->m_contentLayer->addChild(listMenu);

    float currentY = totalHeight;
    for (auto& item : arr) {
        currentY -= itemHeight;
        
        std::string name = item["name"].asString().unwrapOr("Unknown");
        auto label = CCLabelBMFont::create(name.c_str(), "goldFont.fnt");
        label->setScale(0.5f);
        label->setAnchorPoint({0.f, 0.5f});
        label->setPosition(15.f, currentY + 17.f);
        m_scroll->m_contentLayer->addChild(label);

        // Download Button
        auto downloadBtn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_downloadBtn_001.png"),
            this, menu_selector(MacroHubLayer::onDownload)
        );
        downloadBtn->setScale(0.7f);
        downloadBtn->setPosition(285.f, currentY + 17.f);
        
        // Save the dump string and filename in user fields
        downloadBtn->setID(item["macro_data"].dump()); 
        downloadBtn->setUserObject(CCString::create(name));
        
        listMenu->addChild(downloadBtn);
    }
}

void MacroHubLayer::onDownload(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    m_tempDownloadData = btn->getID();
    std::string filename = static_cast<CCString*>(btn->getUserObject())->getCString();

    file::FilePickOptions options;
    options.defaultPath = filename + ".json";
    options.filters = { { "Macro Files", { "*.json", "*.gdr", "*.gdr2" } } };

    m_pickListener.bind([this](geode::Task<geode::utils::file::PickResult>::Event* event) {
        if (auto res = event->getValue()) {
            if (res->isOk()) {
                auto pickResult = res->unwrap(); // geode::Result unwraps to std::optional
                if (pickResult.has_value()) {
                    auto path = pickResult.value();
                    file::writeString(path, m_tempDownloadData);
                    FLAlertLayer::create("Success", "Macro Saved!", "OK")->show();
                }
            }
        }
    });
    m_pickListener.setFilter(file::pick(file::PickMode::SaveFile, options));
}

void MacroHubLayer::onUpload(CCObject*) {
    if (!canUpload()) {
        FLAlertLayer::create("Limit Reached", "You can only upload 3 per day!", "OK")->show();
        return;
    }

    file::FilePickOptions options;
    options.filters = { { "Macro Files", { "*.json", "*.gdr", "*.gdr2" } } };

    m_pickListener.bind([this](geode::Task<geode::utils::file::PickResult>::Event* event) {
        if (auto res = event->getValue()) {
            if (res->isOk()) {
                auto pickResult = res->unwrap();
                if (pickResult.has_value()) {
                    auto path = pickResult.value();
                    auto readRes = file::readString(path);
                    if (!readRes) return;

                    std::string content = readRes.unwrap();
                    std::string fileName = path.stem().string();

                    matjson::Value data;
                    auto parseRes = matjson::parse(content);
                    if (parseRes.isOk()) {
                        data = parseRes.unwrap();
                    } else {
                        // For .gdr/.gdr2 that might be binary, store as raw string
                        data = matjson::Value(content);
                    }

                    MacroManager::uploadMacro(fileName, data, [this](bool success) {
                        if (success) {
                            this->incrementUploadCount();
                            FLAlertLayer::create("Success", "Macro Uploaded!", "OK")->show();
                            this->onRefresh(nullptr);
                        } else {
                            FLAlertLayer::create("Error", "Upload failed.", "OK")->show();
                        }
                    });
                }
            }
        }
    });
    m_pickListener.setFilter(file::pick(file::PickMode::OpenFile, options));
}

bool MacroHubLayer::canUpload() {
    auto mod = Mod::get();
    std::string today = getTodayDateString();
    if (mod->getSavedValue<std::string>("last-upload-date", "") != today) {
        mod->setSavedValue("last-upload-date", today);
        mod->setSavedValue("upload-count", 0);
        return true;
    }
    return mod->getSavedValue<int>("upload-count", 0) < 3;
}

void MacroHubLayer::incrementUploadCount() {
    auto mod = Mod::get();
    int count = mod->getSavedValue<int>("upload-count", 0);
    mod->setSavedValue("upload-count", count + 1);
}

void MacroHubLayer::onClose(CCObject*) {
    this->setKeypadEnabled(false);
    this->removeFromParentAndCleanup(true);
}

void MacroHubLayer::show() {
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    scene->addChild(this, 100);
}
}