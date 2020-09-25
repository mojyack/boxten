/* This is an internal header, which will not be installed. */
#pragma once
#include <filesystem>
#include <mutex>

#include "type.hpp"

namespace boxten::config {
/* for boxten only */
constexpr char boxten_module_name[] = "boxten";

bool set_config_dir(std::filesystem::path config_dir); /* call this first! */
bool get_layout_config(LayoutData& layout);

/* for plugin */
bool get_string(const char* key, std::string& result, const char* module_name = boxten_module_name);
bool get_string_array(const char* key, std::vector<std::string>& result, const char* module_name = boxten_module_name);
void set_string_array(const char* key, const std::vector<std::string>& data, const char* module_name = boxten_module_name);
bool get_number(const char* key, i64& result, const char* module_name = boxten_module_name);
bool get_configuration_file_path(std::filesystem::path path, const char* module_name = boxten_module_name);
} // namespace boxten