#pragma once
#include <mutex>

#include "audiofile.hpp"

namespace boxten {
class Playlist {
    using iterator = std::vector<AudioFile*>::iterator;

  private:
    SafeVar<std::string>             name;
    SafeVar<std::vector<AudioFile*>> playlist_member;

    void proc_insert(std::filesystem::path path, iterator pos);

  public:
    void        set_name(const char* new_name);
    std::string get_name();

    void        activate();
    std::mutex& mutex(); // lock this before call following functions
    iterator    begin();
    iterator    end();
    void        add(std::filesystem::path path);
    void        insert(std::filesystem::path path, iterator pos);
    iterator    erase(iterator pos);
    void        clear();
    u64         size();
    bool        empty();
    AudioFile*  operator[](u64 n);
    Playlist() = default;
    ~Playlist();
};
} // namespace boxten
