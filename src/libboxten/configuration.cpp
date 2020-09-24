#include <fstream>
#include <functional>
#include <iostream>

#include "configuration.hpp"
#include "console.hpp"
#include "debug.hpp"
#include "json.hpp"
#include <config.h>

namespace boxten::config {
namespace {
enum JSON_TYPE {
    OBJECT,
    STRING,
    ARRAY,
};

std::filesystem::path config_home_dir;
std::filesystem::path config_path;
nlohmann::json        config_data;
std::mutex            config_data_lock;

bool type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg ) {
    if(!cfg.contains(key)) {
        DEBUG_OUT(key << "was not found.");
        return false;
    }
    bool result;
    switch(type) {
    case JSON_TYPE::OBJECT:
        result = cfg[key].is_object();
        break;
    case JSON_TYPE::STRING:
        result = cfg[key].is_string();
        break;
    case JSON_TYPE::ARRAY:
        result = cfg[key].is_array();
        break;
    }
    if(!result) {
        DEBUG_OUT("key " << key << " type mismatch.");
    }
    return result;
}
void save_config() {
    std::ofstream config_file(config_path);
    config_file << std::setw(4) << config_data << std::endl;
}
bool get_string(const char* key, std::string& result, const nlohmann::json& cfg) {
    if(!type_check(key, JSON_TYPE::STRING, cfg)) return false;
    result = cfg[key];
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

bool get_component_name(const char* key, ComponentName& result, const nlohmann::json& cfg){
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

bool set_config_dir(std::filesystem::path config_dir){
    if(!std::filesystem::is_directory(config_dir)) return false;
    config_home_dir = config_dir;
    {
        config_path = config_home_dir;
        config_path.append("config.json");
        if(!std::filesystem::is_regular_file(config_path)) return true;
        std::ifstream config_file(config_path);
        config_file >> config_data;
    }
    return true;
}
bool get_string(const char* key, std::string& result) {
    LOCK_GUARD_D(config_data_lock, lock);
    return get_string(key, result, config_data);
}
bool get_string_array(const char* key, std::vector<std::string>& result) {
    LOCK_GUARD_D(config_data_lock, lock);
    return get_string_array(key, result, config_data);
}
void set_string_array(const char* key, const std::vector<std::string>& data){
    LOCK_GUARD_D(config_data_lock, lock);
    config_data[key] = data;
    save_config();
}
bool get_component_name(const char* key, ComponentName& result){
    LOCK_GUARD_D(config_data_lock, lock);
    return get_component_name(key, result, config_data);
}
void set_component_name(const char* key, const ComponentName& data){
    set_string_array(key, std::vector<std::string>{data[0], data[1]});
}
bool get_layout_config(LayoutData& layout) {
    constexpr const char* key = "layout";
    LOCK_GUARD_D(config_data_lock, lock);
    if(!type_check(key, JSON_TYPE::OBJECT, config_data)) return false;

    bool success = true;

    std::function<void(const nlohmann::json&, LayoutData&)> parse_layout = [&parse_layout, &success](const nlohmann::json& cfg, LayoutData& layout) {
        if(std::string type; !get_string("type", type, cfg)) {
            success = false;
            return;
        }else{
            if(type == "vlayout"){
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
} // namespace boxten