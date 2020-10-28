#include <list>

#include <dlfcn.h>

#include "console.hpp"
#include "debug.hpp"
#include "module.hpp"
#include "module_forward.hpp"

#define DO_STATEMENT(fnc, on_success, on_fail) \
if(auto var = fnc; var != nullptr) {           \
        on_success;                            \
    }                                          \
    else {                                     \
        on_fail;                               \
    }

#define FIND_SYM(sym_name, type, found, error_message, on_fail) \
DO_STATEMENT(reinterpret_cast<type>(dlsym(library_handle, sym_name)), found = var, if(error_message != nullptr) DEBUG_OUT(error_message << "(dlsym failed: " << dlerror() << ")"); on_fail)

namespace boxten {
typedef Module* (*InstallModuleFunc)(void);
typedef void (*UninstallModuleFunc)(Module*);

namespace {
class LibraryInfo {
  private:
    void*   library_handle   = nullptr;
    bool    has_module_class = true;
    Module* module_instance;

    InstallModuleFunc   install_module;
    UninstallModuleFunc uninstall_module;
    const char*         exported_module_name;
    ComponentCatalogue* exported_component_catalogue;

    u64 active_component_count = 0;

    void increment_component_count();
    void decrement_component_count();

  public:
    std::pair<Component*, std::function<void(Component* arg)>> find_component(const ComponentName& component_name);
    LibraryInfo(const char* path);
    ~LibraryInfo();
};
} // namespace

void LibraryInfo::increment_component_count(){
    if(active_component_count == 0) {
        if(has_module_class) module_instance = (*install_module)();
    }
    active_component_count++;
}
void LibraryInfo::decrement_component_count(){
    active_component_count--;
    if(active_component_count == 0) {
        if(has_module_class) (*uninstall_module)(module_instance);
    }
}
std::pair<Component*, std::function<void(Component* arg)>> LibraryInfo::find_component(const ComponentName& component_name) {
    for(auto c : *exported_component_catalogue) {
        if(exported_module_name == component_name[0] &&  c.name == component_name[1]) {
            increment_component_count();
            auto param = ComponentConstructionParam(std::string(exported_module_name), c, std::bind(&LibraryInfo::decrement_component_count, this));
            auto component = c.alloc(&param);
            return std::pair(component, c.free);
        }
    }
    return std::pair(nullptr, nullptr);
}
LibraryInfo::LibraryInfo(const char* path) {
    DO_STATEMENT(dlopen(path, RTLD_LAZY), library_handle = var,
                 DEBUG_OUT("cannot open library file(dlopen failed: " << dlerror() << ")");
                 throw;);
    {
        FIND_SYM("install_module", InstallModuleFunc, install_module, nullptr, has_module_class = false);
        FIND_SYM("uninstall_module", UninstallModuleFunc, uninstall_module, nullptr, has_module_class = false);
    }
    do{
        FIND_SYM("module_name", const char*, exported_module_name, "cannot find module_name", break);
        FIND_SYM("component_catalogue", ComponentCatalogue*, exported_component_catalogue, "cannot find component_catalogue", break);
        return;
    } while(0);
    dlclose(library_handle);
    throw;
}
LibraryInfo::~LibraryInfo() {
    if(active_component_count != 0){;
        DEBUG_OUT("closing module \"" << exported_module_name << "\" which has active component.");
    }
    dlclose(library_handle);
}

namespace{
std::list<LibraryInfo*>                                                 libraries;
std::vector<std::pair<Component*, std::function<void(Component* arg)>>> active_components;
} // namespace

u64 scan_modules(std::vector<std::filesystem::path> lib_dirs) {
    for(auto module_dir : lib_dirs) {
        for(const std::filesystem::directory_entry& m : std::filesystem::directory_iterator(module_dir)) {
            LibraryInfo* lib;
            try {
                lib = new LibraryInfo(m.path().string().data());
            } catch(...) {
                DEBUG_OUT("an essention function not found while.");
                continue;
            }
            libraries.emplace_back(lib);
        }
    }
    return libraries.size();
}
void free_modules(){
    for(auto& lib : libraries) {
        delete lib;
    }
    libraries.clear();
}
Component* search_component(ComponentName name) {
    for(auto lib : libraries) {
        if(auto c = lib->find_component(name); c.first != nullptr) {
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