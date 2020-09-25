/* This is an internal header, which will not be installed. */
#pragma once
#include <filesystem>
#include <mutex>

#include "type.hpp"

namespace boxten::config {
/* for boxten */
inline const ComponentName boxten_component_name = {"boxten", "boxten"};

bool set_config_dir(std::filesystem::path config_dir); /* call this first! */
bool get_layout_config(LayoutData& layout);

/* for plugin */
bool get_string(const char* key, std::string& result, const ComponentName& component_name = boxten_component_name);
bool get_string_array(const char* key, std::vector<std::string>& result, const ComponentName& component_name = boxten_component_name);
void set_string_array(const char* key, const std::vector<std::string>& data, const ComponentName& component_name = boxten_component_name);
} // namespace boxten