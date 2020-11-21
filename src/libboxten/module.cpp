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
    std::filesystem::path library_path;
    void*                 library_handle = nullptr;
    std::string           module_name;
    struct SimpleComponentCatalogue {
        std::string    name;
        COMPONENT_TYPE type;
    };
    std::vector<SimpleComponentCatalogue> component_catalogue;
    
    bool                                  has_module_class = true;
    Module*                               module_instance;

    InstallModuleFunc   install_module;
    UninstallModuleFunc uninstall_module;
    ComponentCatalogue* exported_component_catalogue;

    u64 active_component_count = 0;

    void load_library();
    void unload_library();
    void create_module();
    void destroy_module();
    void increment_component_count();
    void decrement_component_count();

  public:
    std::pair<Component*, std::function<void(Component* arg)>> find_component(const ComponentName& component_name);
    void                                                       check_and_unload_library();
    LibraryInfo(const char* path);
    ~LibraryInfo();
};
std::list<LibraryInfo*>                                                 libraries;
std::vector<std::pair<Component*, std::function<void(Component* arg)>>> active_components;
} // namespace

void LibraryInfo::load_library(){
    DO_STATEMENT(dlopen(library_path.string().data(), RTLD_LAZY), library_handle = var,
                 DEBUG_OUT("cannot open library file(dlopen failed: " << dlerror() << ")");
                 throw;);
}
void LibraryInfo::unload_library(){
    if(active_component_count != 0) {
        DEBUG_OUT("closing module \"" << module_name << "\" which has active component.");
    }
    dlclose(library_handle);
    library_handle = nullptr;
}
void LibraryInfo::create_module() {
    has_module_class = true;
    FIND_SYM("install_module", InstallModuleFunc, install_module, nullptr, has_module_class = false);
    FIND_SYM("uninstall_module", UninstallModuleFunc, uninstall_module, nullptr, has_module_class = false);
    if(has_module_class) module_instance = (*install_module)();
}
void LibraryInfo::destroy_module() {
    if(has_module_class) (*uninstall_module)(module_instance);
}
void LibraryInfo::increment_component_count(){
    if(active_component_count == 0) {
        load_library();
        create_module();
        FIND_SYM("component_catalogue", ComponentCatalogue*, exported_component_catalogue, nullptr, );
    }
    active_component_count++;
}
void LibraryInfo::decrement_component_count(){
    active_component_count--;
    if(active_component_count == 0) {
        destroy_module();
    }
}
std::pair<Component*, std::function<void(Component* arg)>> LibraryInfo::find_component(const ComponentName& component_name) {
    for(auto c = component_catalogue.begin(); c != component_catalogue.end();++c) {
        if(module_name == component_name[0] && c->name == component_name[1]) {
            increment_component_count();
            ComponentInfo& component_info = (*exported_component_catalogue)[std::distance(component_catalogue.begin(), c)];
            auto           param          = ComponentConstructionParam(module_name, component_info, std::bind(&LibraryInfo::decrement_component_count, this));
            auto           component      = component_info.alloc(&param);
            return std::pair(component, component_info.free);
        }
    }
    return std::pair(nullptr, nullptr);
}
void LibraryInfo::check_and_unload_library() {
    if(library_handle != nullptr && active_component_count == 0) {
        unload_library();
    }
}
LibraryInfo::LibraryInfo(const char* path):library_path(path) {
    load_library();
    bool error = true;
    do {
        FIND_SYM("module_name", const char*, module_name, "cannot find module_name", break);
        /* create simple catalogue */
        ComponentCatalogue* exported_component_catalogue;
        FIND_SYM("component_catalogue", ComponentCatalogue*, exported_component_catalogue, "cannot find component_catalogue", break);
        for(auto& c : *exported_component_catalogue){
            component_catalogue.emplace_back(c.name, c.type);
        }
        error = false;
    } while(0);
    unload_library();
    if(!error)
        return;
    else
        throw;
}
LibraryInfo::~LibraryInfo() {
    if(library_handle != nullptr) unload_library();
}

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
void free_inactive_modules(){
    for(auto& lib : libraries) {
        lib->check_and_unload_library();
    }
}
Component* search_component(ComponentName name) {
    for(auto& lib : libraries) {
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
        free_inactive_modules();
        return true;
    }
    return false;
}
} // namespace boxten