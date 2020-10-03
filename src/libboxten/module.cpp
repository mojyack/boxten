#include <list>

#include <dlfcn.h>

#include "console.hpp"
#include "debug.hpp"
#include "module.hpp"
#include "module_forward.hpp"

namespace boxten {
bool LibraryInfo::open_library(){
    if(library_handle != nullptr) return true;

    if(auto handle = dlopen(library_path.string().data(), RTLD_LAZY); handle != NULL) {
        library_handle = handle;
    } else {
        DEBUG_OUT("cannot open library file(dlopen failed: " << dlerror() << ")");
        return false;
    }
    if(auto module = reinterpret_cast<Module*>(dlsym(library_handle, "module_instance")); module != nullptr) {
        module_instance = module;
    } else {
        DEBUG_OUT("cannot find module instance(dlsym failed: " << dlerror() << ")");
        close_library();
        return false;
    }
    component_catalogue.clear();
    for(const auto& c : module_instance->component_catalogue){
        component_catalogue.emplace_back(c.name);
    }
    return true;
}
bool LibraryInfo::close_library(){
    if(library_handle == nullptr) return true;

    dlclose(library_handle);
    library_handle = nullptr;
    module_instance = nullptr;
    return true;
}
void LibraryInfo::increment_component_count(){
    if(active_component_count == 0) open_library();
    active_component_count++;
}
void LibraryInfo::decrement_component_count(){
    active_component_count--;
    if(active_component_count == 0) close_library();
}
bool LibraryInfo::initialize() {
    if(initialized) return true;

    const bool temporary_open = active_component_count == 0;
    if(temporary_open && !open_library()) return false;

    module_name         = module_instance->module_name;

    if(temporary_open && !close_library()) return false;

    initialized = true;
    return true;
}
std::pair<Component*, std::function<void(Component* arg)>> LibraryInfo::find_component(const ComponentName& component_name) {
    bool found = false;
    for(auto c : component_catalogue) {
        if(module_name == component_name[0] && c == component_name[1]) {
            found = true;
            break;
        }
    }
    if(!found) return std::pair(nullptr, nullptr);

    increment_component_count();

    for(auto c : module_instance->component_catalogue) {
        if(module_name == component_name[0] &&  c.name == component_name[1]) {
            auto param = ComponentConstructionParam(std::string(module_name), c, std::bind(&LibraryInfo::decrement_component_count, this));
            auto component = c.alloc(&param);
            return std::pair(component, c.free);
        }
    }

    /* We should not reach here. */
    return std::pair(nullptr, nullptr);
}
LibraryInfo::LibraryInfo(std::filesystem::path path) : library_path(path) {}

namespace{
std::list<LibraryInfo>                                                  libraries;
std::vector<std::pair<Component*, std::function<void(Component* arg)>>> active_components;
} // namespace

u64 scan_modules(std::vector<std::filesystem::path> lib_dirs) {
    for(auto module_dir : lib_dirs) {
        for(const std::filesystem::directory_entry& m : std::filesystem::directory_iterator(module_dir)) {
            if(LibraryInfo library(m.path()); library.initialize()){
                libraries.emplace_back(library);
            }
        }
    }
    return libraries.size();
}
Component* search_component(ComponentName name) {
    for(auto lib : libraries) {
        if(auto c = lib.find_component(name); c.first != nullptr) {
            active_components.emplace_back(c);
            return c.first;
        }
    }
    return nullptr;
}
bool close_component(boxten::Component* component){
    for(auto c = active_components.begin(); c != active_components.end();++c){
        if(c->first != component) continue;
        c->second(c->first);
        active_components.erase(c);
        return true;
    }
    return false;
}
} // namespace boxten