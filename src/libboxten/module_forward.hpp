#pragma once
#include <string>
#include "plugin.hpp"


namespace boxten {
struct ComponentConstructionParam {
    std::string   module_name;
    ComponentInfo info;
    ComponentConstructionParam(std::string module_name, ComponentInfo info) : module_name(module_name), info(info) {}
};
} // namespace boxten