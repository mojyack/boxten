#pragma once
#include <array>
#include <filesystem>
#include <vector>
#include <string>

#include "gui.hpp"

std::filesystem::path              find_config_dir();
std::vector<std::filesystem::path> get_module_dirs();
std::array<std::string, 2>         get_input_component();
std::array<std::string, 2>         get_output_component();
bool                               load_layout(BaseWindow& base_window);