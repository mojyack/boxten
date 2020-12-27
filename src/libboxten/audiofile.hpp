#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>

#include "type.hpp"

namespace boxten{
class Playlist;
class StreamInput;

class AudioFile {
  private:
    const std::filesystem::path path;
    std::ifstream               handle;

    n_frames                   total_frames = 0;

    StreamInput*               input_module_private_data_owner = nullptr;
    void*                      input_module_private_data = nullptr;
    std::function<void(void*)> input_module_private_data_deleter;
    void                       free_input_module_private_data();

    bool     tags_updated = false;
    AudioTag tags;

  public:
    std::ifstream&        get_handle();
    std::filesystem::path get_path();
    void                  set_private_data(void* data, StreamInput* owner, std::function<void(void*)> deleter);
    void*                 get_private_data();
    bool                  cleanup_private_data(StreamInput* stream_input);
    n_frames get_total_frames();
    AudioTag get_tags();

    AudioFile(std::filesystem::path path) : path(path) {}
    ~AudioFile();
};
} // namespace boxten
