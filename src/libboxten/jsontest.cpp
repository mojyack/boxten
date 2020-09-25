#include "jsontest.hpp"

namespace boxten {
namespace {
bool proc_type_check(JSON_TYPE type, const nlohmann::json& cfg) {
    bool result;
    switch(type) {
    case JSON_TYPE::OBJECT:
        result = cfg.is_object();
        break;
    case JSON_TYPE::STRING:
        result = cfg.is_string();
        break;
    case JSON_TYPE::ARRAY:
        result = cfg.is_array();
        break;
    }
    return result;
}
} // namespace
bool type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg) {
    if(!cfg.contains(key)) {
        return false;
    }
    return proc_type_check(type, cfg[key]);
}
bool array_type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg) {
    auto array = type_check(key, JSON_TYPE::ARRAY, cfg);
    if(!array) return false;
    for(auto& c : cfg[key]) {
        if(!proc_type_check(type, c)) return false;
    }
    return true;
}
} // namespace boxten