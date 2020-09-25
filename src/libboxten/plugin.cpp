#include "plugin.hpp"
#include "config.h"
#include "configuration.hpp"
#include "eventhook_internal.hpp"
#include "module_forward.hpp"
#include "playback_internal.hpp"

namespace boxten{
bool Configurator::get_string(const char* key, std::string& result) {
    return boxten::config::get_string(key, result, domain.data());
}
bool Configurator::get_string_array(const char* key, std::vector<std::string>& result) {
    return boxten::config::get_string_array(key, result, domain.data());
}
void Configurator::set_string_array(const char* key, const std::vector<std::string>& data) {
    boxten::config::set_string_array(key, data, domain.data());
}
bool Configurator::get_configuration_file_path(std::filesystem::path& path) {
    return boxten::config::get_configuration_file_path(path, domain.data());
}
Configurator::Configurator(const char* domain):domain(domain){}

std::filesystem::path Component::get_resource_dir(){
    if(resource_dir.empty()) {
        resource_dir = DATADIR;
        resource_dir /= component_name[0];
    }
    return resource_dir;
}
void Component::install_eventhook(std::function<void(void)> hook, std::initializer_list<EVENT> events) {
    for(auto e:events){
        boxten::install_eventhook(hook, e, this);
    }
}
Component::Component(void* param) : 
    component_name({
        reinterpret_cast<ComponentConstructionParam*>(param)->module_name,reinterpret_cast<ComponentConstructionParam*>(param)->info.name}),
    component_type(reinterpret_cast<ComponentConstructionParam*>(param)->info.type),
    free(reinterpret_cast<ComponentConstructionParam*>(param)->info.free),
    Configurator(component_name[0].data()) {
}
Component::~Component(){
    uninstall_eventhook(this);
}

n_frames StreamOutput::output_delay() {
    return 0;
}
n_frames StreamOutput::get_buffer_filled_frames(){
    return boxten::get_buffer_filled_frames();
}
PCMPacket StreamOutput::get_buffer_pcm_packet(n_frames frame) {
    return boxten::get_buffer_pcm_packet(frame);
}
PCMFormat StreamOutput::get_buffer_pcm_format(){
    return boxten::get_buffer_pcm_format();
}
Module::Module(const char* module_name) : module_name(module_name), Configurator(module_name) {}
} // namespace boxten