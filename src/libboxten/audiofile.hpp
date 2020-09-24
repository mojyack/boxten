#pragma once
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>

#include "type.hpp"

namespace boxten{
class Playlist;

class AudioFile {
  private:
    const std::filesystem::path path;
    std::ifstream               handle;

    n_frames                   total_frames = 0;

    void*                      input_module_private_data = nullptr;
    std::function<void(void*)> input_module_private_data_deleter;
    void                       free_input_module_private_data();

    std::map<std::string, std::string> tags;

  public:
    std::ifstream&        get_handle();
    std::filesystem::path get_path();
    void                  set_private_data(void* data, std::function<void(void*)> deleter);
    void*                 get_private_data();

    n_frames get_total_frames();

    AudioFile(std::filesystem::path path) : path(path) {}
    ~AudioFile();
};
} // namespace boxten