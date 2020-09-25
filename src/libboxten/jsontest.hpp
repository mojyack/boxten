#pragma once
#include "json.hpp"

namespace boxten{
enum JSON_TYPE {
    OBJECT,
    STRING,
    ARRAY,
};
bool type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg);
bool array_type_check(const char* key, JSON_TYPE type, const nlohmann::json& cfg);
}