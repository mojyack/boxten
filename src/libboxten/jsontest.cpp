#include "jsontest.hpp"

namespace boxten{
bool type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg) {
    if(!cfg.contains(key)) {
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
    return result;
}
}