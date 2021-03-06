#include "plugin.hpp"
#include "config.h"
#include "configuration.hpp"
#include "console.hpp"
#include "debug.hpp"
#include "eventhook_internal.hpp"
#include "module_forward.hpp"
#include "playback_internal.hpp"
#include "playlist_internal.hpp"

namespace boxten {
namespace {
inline std::string build_console_prefix(const std::string& name) {
    std::string prefix = "[" + name + "] ";
    return prefix;
}
} // namespace
std::filesystem::path Component::get_resource_dir() {
    if(resource_dir.empty()) {
        resource_dir = DATADIR;
        resource_dir /= component_name[0].data();
    }
    return resource_dir;
}
void Component::install_eventhook(HookFunction hook, Events event) {
    boxten::install_eventhook(hook, event, this);
}
void Component::install_eventhook(HookFunction hook, std::initializer_list<Events> events) {
    for(auto e : events) {
        boxten::install_eventhook(hook, e, this);
    }
}
bool Component::get_string(const char* key, std::string& result) {
    return boxten::config::get_string(key, result, component_name[0].data());
}
bool Component::get_string_array(const char* key, std::vector<std::string>& result) {
    return boxten::config::get_string_array(key, result, component_name[0].data());
}
void Component::set_string_array(const char* key, const std::vector<std::string>& data) {
    boxten::config::set_string_array(key, data, component_name[0].data());
}
bool Component::get_number(const char* key, i64& result) {
    return boxten::config::get_number(key, result, component_name[0].data());
}
void Component::set_number(const char* key, i64 val) {
    boxten::config::set_number(key, val, component_name[0].data());
}
bool Component::load_configuration(nlohmann::json& config_data) {
    return boxten::config::load_configuration(config_data, component_name[0].data());
}
bool Component::save_configuration(const nlohmann::json& config_data) {
    return boxten::config::save_configuration(config_data, component_name[0].data());
}
Component::Component(void* param) : component_name({reinterpret_cast<ComponentConstructionParam*>(param)->module_name, reinterpret_cast<ComponentConstructionParam*>(param)->info.name}),
                                    component_type(reinterpret_cast<ComponentConstructionParam*>(param)->info.type),
                                    decrement_component_count(reinterpret_cast<ComponentConstructionParam*>(param)->decrement_component_count),
                                    console(build_console_prefix(component_name[0]).data()) {}
Component::~Component() {
    uninstall_eventhook(this);
    decrement_component_count();
    DEBUG_OUT("component \"" << component_name[1] << "\" closed.");
}

StreamInput::~StreamInput() {
    cleanup_private_data(this);
}

n_frames StreamOutput::output_delay() {
    return 0;
}
n_frames StreamOutput::get_buffer_filled_frames() {
    return boxten::get_buffer_filled_frames();
}
PCMPacket StreamOutput::get_buffer_pcm_packet(n_frames frame) {
    return boxten::get_buffer_pcm_packet(frame);
}
PCMFormat StreamOutput::get_buffer_pcm_format() {
    return boxten::get_buffer_pcm_format();
}
} // namespace boxten
