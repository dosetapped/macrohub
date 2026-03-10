#pragma once
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <functional>

using namespace geode::prelude;

class MacroManager {
public:
    static void fetchMacros(std::function<void(matjson::Value)> callback);
    static void uploadMacro(std::string const& name, matjson::Value const& data, std::function<void(bool)> callback);
};