#pragma once
#include <array>
#include <filesystem>
#include <vector>
#include <string>

#include <module.hpp>
#include <type.hpp>

#include "gui.hpp"

std::filesystem::path find_config_dir();
bool                  get_module_dirs(std::vector<std::filesystem::path>& dirs);
bool                  get_input_component(boxten::ComponentName& c_name);
bool                  get_output_component(boxten::ComponentName& c_name);
bool                  apply_layout(BaseWindow& base_window, boxten::LayoutData layout);