#pragma once
#include <string>
#include "plugin.hpp"


namespace boxten {
struct ComponentConstructionParam {
    std::string               module_name;
    ComponentInfo             info;
    std::function<void(void)> decrement_component_count;
    ComponentConstructionParam(std::string module_name, ComponentInfo info, std::function<void(void)> on_destruction) : module_name(module_name), info(info), decrement_component_count(on_destruction) {}
};
} // namespace boxten