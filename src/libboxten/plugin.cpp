#include "plugin.hpp"
#include "config.h"
#include "configuration.hpp"
#include "eventhook_internal.hpp"
#include "module_forward.hpp"
#include "playback_internal.hpp"

namespace boxten{
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
bool Component::get_string(const char* key, std::string& result){
    return boxten::config::get_string(key, result, component_name[0].data());
}
bool Component::get_string_array(const char* key, std::vector<std::string>& result){
    return boxten::config::get_string_array(key, result, component_name[0].data());
}
void Component::set_string_array(const char* key, const std::vector<std::string>& data){
    boxten::config::set_string_array(key, data, component_name[0].data());
}

Component::Component(void* param) : 
    component_name({
        reinterpret_cast<ComponentConstructionParam*>(param)->module_name,reinterpret_cast<ComponentConstructionParam*>(param)->info.name}),
    component_type(reinterpret_cast<ComponentConstructionParam*>(param)->info.type),
    free(reinterpret_cast<ComponentConstructionParam*>(param)->info.free) {
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

bool Module::get_string(const char* key, std::string& result) {
    return boxten::config::get_string(key, result, module_name);
}
bool Module::get_string_array(const char* key, std::vector<std::string>& result) {
    return boxten::config::get_string_array(key, result, module_name);
}
void Module::set_string_array(const char* key, const std::vector<std::string>& data) {
    boxten::config::set_string_array(key, data, module_name);
}
}