/* This is an internal header, which will not be installed. */
#pragma once
#include <filesystem>
#include <mutex>

#include "plugin.hpp"

namespace boxten::config {
bool set_config_dir(std::filesystem::path config_dir); /* call this first! */
bool get_string(const char* key, std::string& result);
bool get_string_array(const char* key, std::vector<std::string>& result);
void set_string_array(const char* key, const std::vector<std::string>& data);

bool get_component_name(const char* key, ComponentName& result);
void set_component_name(const char* key, const ComponentName& data);
bool get_layout_config(LayoutData& layout);
} // namespace boxten