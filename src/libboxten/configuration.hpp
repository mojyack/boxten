/* This is an internal header, which will not be installed. */
#pragma once
#include <filesystem>
#include <mutex>

#include "json.hpp"

namespace boxten::config {
bool            set_config_dir(std::filesystem::path config_dir); /* call this first! */
nlohmann::json& get_configuration_ref();
std::mutex&     get_configuration_lock_ref();
void            save_config();
} // namespace boxten