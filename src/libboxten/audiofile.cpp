#include "audiofile.hpp"
#include "playback_internal.hpp"

namespace boxten{
void AudioFile::free_input_module_private_data(){
    if(input_module_private_data == nullptr ||
       !input_module_private_data_deleter) return;
    input_module_private_data_deleter(input_module_private_data);
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

void AudioFile::set_private_data(void* data, std::function<void(void*)> deleter){
    if(input_module_private_data != nullptr) free_input_module_private_data();
    input_module_private_data         = data;
    input_module_private_data_deleter = deleter;
}
void* AudioFile::get_private_data(){
    return input_module_private_data;
}
n_frames AudioFile::get_total_frames(){
    if(total_frames == 0) {
        total_frames = boxten::get_total_frames(this);
    }
    return total_frames;
}

AudioFile::~AudioFile(){
    free_input_module_private_data();
    if(handle.is_open()) handle.close();
}
}