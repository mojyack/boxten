#include <fstream>
#include <functional>
#include <iostream>

#include "configuration.hpp"
#include "console.hpp"
#include "debug.hpp"
#include "json.hpp"
#include "jsontest.hpp"
#include <config.h>

namespace boxten::config {
namespace {

std::filesystem::path config_home_dir;
std::filesystem::path config_path;

void save_config(const std::filesystem::path path, const nlohmann::json& cfg) {
    std::ofstream handle(path);
    handle << std::setw(4) << cfg << std::endl;
}
bool load_config_file(const std::filesystem::path path, nlohmann::json& cfg) {
    if(!std::filesystem::is_regular_file(path)) return false;
    std::ifstream handle(path);
    handle >> cfg;
    return true;
}
bool get_config_path(const char* module_name, std::filesystem::path& path) {
    path = config_home_dir;
    if(std::strcmp(module_name, boxten_module_name) != 0) path /= module_name;
    if(!std::filesystem::exists(path)) {
        std::filesystem::create_directory(path);
    } else if(!std::filesystem::is_directory(path)) {
        return false;
    }
    path /= "config.json";
    return true;
}

bool get_string(const char* key, std::string& result, const nlohmann::json& cfg) {
    if(!type_check(key, JSON_TYPE::STRING, cfg)) return false;
    result = cfg[key].get<std::string>();
    return true;
}
bool get_string_array(const char* key, std::vector<std::string>& result, const nlohmann::json& cfg) {
    if(!type_check(key, JSON_TYPE::ARRAY, cfg)) return false;
    for(auto& s : cfg[key]) {
        if(!s.is_string()) {
            DEBUG_OUT(key << "contains non-string member.");
        } else {
            result.emplace_back(s.get<std::string>());
        }
    }
    return true;
}
bool get_number(const char* key, i64& result, const nlohmann::json& cfg) {
    if(!type_check(key, JSON_TYPE::NUMBER, cfg)) return false;
    result = cfg[key].get<i64>();
    return true;
}

bool get_component_name(const char* key, ComponentName& result, const nlohmann::json& cfg) {
    std::vector<std::string> name;
    if(!get_string_array(key, name, cfg) || name.size() != 2) {
        DEBUG_OUT(key << " is not module name.");
        return false;
    }
    result[0] = name[0];
    result[1] = name[1];
    return true;
}
} // namespace

bool set_config_dir(std::filesystem::path config_dir) {
    if(!std::filesystem::is_directory(config_dir)) return false;
    config_home_dir = config_dir;
    return get_config_path(boxten_module_name, config_path);
}
bool get_layout_config(LayoutData& layout) {
    constexpr const char* key = "layout";
    nlohmann::json        config_data;
    if(!load_config_file(config_path, config_data)) return false;
    if(!type_check(key, JSON_TYPE::OBJECT, config_data)) return false;

    bool success = true;

    std::function<void(const nlohmann::json&, LayoutData&)> parse_layout = [&parse_layout, &success](const nlohmann::json& cfg, LayoutData& layout) {
        if(std::string type; !get_string("type", type, cfg)) {
            success = false;
            return;
        } else {
            if(type == "vlayout") {
                layout.type = LayoutData::V_SPLIT;
            } else if(type == "hlayout") {
                layout.type = LayoutData::H_SPLIT;
            } else if(type == "widget") {
                layout.type = LayoutData::WIDGET;
            } else {
                DEBUG_OUT("key \"type\" is invalid");
                success = false;
                return;
            }
        }

        if(layout.type == LayoutData::WIDGET) {
            if(!get_component_name("name", layout.name, cfg)) {
                success = false;
                return;
            }
        } else {
            if(!type_check("children", JSON_TYPE::ARRAY, cfg)) {
                success = false;
                return;
            }
            for(auto c : cfg["children"]) {
                parse_layout(c, layout.children.emplace_back());
            }
        }
    };
    parse_layout(config_data[key], layout);

    if(success)
        return true;
    else
        return false;
}

bool get_string(const char* key, std::string& result, const char* module_name) {
    nlohmann::json        config_data;
    std::filesystem::path path;
    if(!get_config_path(module_name, path) || !load_config_file(path, config_data)) return false;
    return get_string(key, result, config_data);
}
bool get_string_array(const char* key, std::vector<std::string>& result, const char* module_name) {
    nlohmann::json        config_data;
    std::filesystem::path path;
    if(!get_config_path(module_name, path) || !load_config_file(path, config_data)) return false;
    return get_string_array(key, result, config_data);
}
void set_string_array(const char* key, const std::vector<std::string>& data, const char* module_name) {
    std::filesystem::path path;
    if(!get_config_path(module_name, path)) return;
    nlohmann::json config_data;
    load_config_file(path, config_data);
    config_data[key] = data;
    save_config(path, config_data);
}
bool get_number(const char* key, i64& result, const char* module_name) {
    nlohmann::json        config_data;
    std::filesystem::path path;
    if(!get_config_path(module_name, path) || !load_config_file(path, config_data)) return false;
    return get_number(key, result, config_data);
}
void set_number(const char* key, i64 data, const char* module_name) {
    std::filesystem::path path;
    if(!get_config_path(module_name, path)) return;
    nlohmann::json config_data;
    load_config_file(path, config_data);
    config_data[key] = data;
    save_config(path, config_data);
}
bool load_configuration(nlohmann::json& result, const char* module_name) {
    std::filesystem::path path;
    if(!get_config_path(module_name, path)) return false;
    load_config_file(path, result);
    return true;
}
bool save_configuration(const nlohmann::json& config_data, const char* module_name) {
    std::filesystem::path path;
    if(!get_config_path(module_name, path)) return false;
    save_config(path, config_data);
    return true;
}
} // namespace boxten::config
