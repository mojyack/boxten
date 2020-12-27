#include "audiofile.hpp"
#include "playback_internal.hpp"

namespace boxten {
void AudioFile::free_input_module_private_data() {
    if(input_module_private_data_owner == nullptr) return;
    input_module_private_data_deleter(input_module_private_data);
    input_module_private_data_owner   = nullptr;
    input_module_private_data         = nullptr;
    input_module_private_data_deleter = nullptr;
}
std::ifstream& AudioFile::get_handle() {
    if(!handle.is_open()) {
        handle.open(path);
    }
    return handle;
}
std::filesystem::path AudioFile::get_path() {
    return path;
}

void AudioFile::set_private_data(void* data, StreamInput* owner, std::function<void(void*)> deleter) {
    if(input_module_private_data_owner != nullptr) free_input_module_private_data();
    input_module_private_data_owner   = owner;
    input_module_private_data         = data;
    input_module_private_data_deleter = deleter;
}
void* AudioFile::get_private_data() {
    return input_module_private_data;
}
n_frames AudioFile::get_total_frames() {
    if(total_frames == 0) {
        total_frames = boxten::get_total_frames(this);
    }
    return total_frames;
}
AudioTag AudioFile::get_tags() {
    if(!tags_updated) {
        tags         = boxten::get_tags(this);
        tags_updated = true;
    }
    return tags;
}
bool AudioFile::cleanup_private_data(StreamInput* stream_input) {
    if(input_module_private_data_owner == stream_input) {
        free_input_module_private_data();
        return true;
    }
    return false;
}
AudioFile::~AudioFile() {
    free_input_module_private_data();
    if(handle.is_open()) handle.close();
}
} // namespace boxten
