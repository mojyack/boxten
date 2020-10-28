/* This is an internal header, which will not be installed. */
#pragma once
#include <array>
#include <filesystem>
#include <vector>

#include "plugin.hpp"
#include "type.hpp"

namespace boxten {
u64                scan_modules(std::vector<std::filesystem::path> lib_dirs);
void               free_modules();
boxten::Component* search_component(ComponentName name);
bool               close_component(boxten::Component* component);
} // namespace boxten