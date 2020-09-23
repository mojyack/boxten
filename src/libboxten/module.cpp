#include <dlfcn.h>

#include "console.hpp"
#include "module.hpp"
#include "module_forward.hpp"

namespace boxten {
namespace {
std::vector<Module*> modules;
} // namespace

u64 open_modules(std::vector<std::filesystem::path> lib_dirs) {
    for(auto module_dir : lib_dirs) {
        for(const std::filesystem::directory_entry& m : std::filesystem::directory_iterator(module_dir)) {
            auto lib = dlopen(m.path().string().data(), RTLD_LAZY);
            if(lib == NULL) {
                console << "dlopen failed: " << dlerror() << std::endl;
                continue;
            }
            auto install_modules = (std::vector<Module*>(*)())dlsym(lib, "install_modules");
            if(install_modules == NULL) {
                console << "dlsym failed: " << dlerror() << std::endl;
                continue;
            }
            auto new_mods = (*install_modules)();
            std::copy(new_mods.begin(), new_mods.end(), std::back_inserter(modules));
        }
    }
    return modules.size();
}
Component* search_component(ComponentName name) {
    for(auto m : modules) {
        if(m->module_name != name[0]) continue;
        for(auto c : m->component_catalogue) {
            if(c.name != name[1]) continue;
            auto param     = ComponentConstructionParam(std::string(m->module_name), c);
            auto component = c.alloc(&param);
            return component;
        }
    }
    return nullptr;
}
} // namespace boxten