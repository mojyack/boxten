#pragma once
#include <string>
#include "plugin.hpp"


namespace boxten {
class LibraryInfo {
    typedef void* LibraryHandle;

  private:
    bool                        initialized = false;
    const std::filesystem::path library_path;
    LibraryHandle               library_handle  = nullptr;
    Module*                     module_instance = nullptr;
    std::string                 module_name;
    std::vector<std::string>    component_catalogue;
    u64                         active_component_count = 0;

    bool open_library();
    bool close_library();
    void increment_component_count();
    void decrement_component_count();

  public:
    bool       initialize();
    std::pair<Component*, std::function<void(Component* arg)>> find_component(const ComponentName& component_name);
    LibraryInfo(std::filesystem::path path);
};
struct ComponentConstructionParam {
    std::string               module_name;
    ComponentInfo             info;
    std::function<void(void)> decrement_component_count;
    ComponentConstructionParam(std::string module_name, ComponentInfo info, std::function<void(void)> on_destruction) : module_name(module_name), info(info), decrement_component_count(on_destruction) {}
};
} // namespace boxten