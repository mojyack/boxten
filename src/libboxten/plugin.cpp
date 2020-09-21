#include "plugin.hpp"
#include "config.h"
#include "module_forward.hpp"
#include "playback_internal.hpp"
#include "eventhook_internal.hpp"

namespace boxten{
std::filesystem::path Component::get_resource_dir(){
    if(resource_dir.empty()) {
        resource_dir = DATADIR;
        resource_dir /= module_name;
    }
    return resource_dir;
}
void Component::install_eventhook(std::function<void(void)> hook, std::initializer_list<EVENT> events) {
    for(auto e:events){
        boxten::install_eventhook(hook, e, this);
    }
}

Component::Component(void* param) : 
component_name(reinterpret_cast<ComponentConstructionParam*>(param)->info.name),
component_type(reinterpret_cast<ComponentConstructionParam*>(param)->info.type),
module_name(reinterpret_cast<ComponentConstructionParam*>(param)->module_name),
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
}