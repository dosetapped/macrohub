#include "MacroManager.hpp"
#include <thread>

void MacroManager::fetchMacros(std::function<void(matjson::Value)> callback) {
    std::thread([callback]() {
        auto req = web::WebRequest();
        req.header("apikey", "sb_publishable_bHzd7kGFlsEsjRM0OHYq2w_K7I38Ojw");
        req.header("Authorization", "Bearer sb_publishable_bHzd7kGFlsEsjRM0OHYq2w_K7I38Ojw");

        auto res = req.sendSync("GET", "https://fprwjbjfullmhimmkuez.supabase.co/rest/v1/macros?select=*");

        if (res.ok()) {
            auto val = res.json().unwrapOr(matjson::Value());
            geode::Loader::get()->queueInMainThread([callback, val]() {
                callback(val);
            });
        } else {
            geode::Loader::get()->queueInMainThread([callback]() {
                callback(matjson::Value());
            });
        }
    }).detach();
}

void MacroManager::uploadMacro(std::string const& name, matjson::Value const& data, std::function<void(bool)> callback) {
    std::thread([name, data, callback]() {
        auto req = web::WebRequest();
        req.header("apikey", "sb_publishable_bHzd7kGFlsEsjRM0OHYq2w_K7I38Ojw");
        req.header("Authorization", "Bearer sb_publishable_bHzd7kGFlsEsjRM0OHYq2w_K7I38Ojw");
        req.header("Content-Type", "application/json");

        matjson::Value body;
        body["name"] = name;
        body["macro_data"] = data;

        req.bodyJSON(body);

        auto res = req.sendSync("POST", "https://fprwjbjfullmhimmkuez.supabase.co/rest/v1/macros");
        bool ok = res.ok();

        geode::Loader::get()->queueInMainThread([callback, ok]() {
            callback(ok);
        });
    }).detach();
}