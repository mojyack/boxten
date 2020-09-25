#pragma once
#include <mutex>

#include "audiofile.hpp"

namespace boxten {
class Playlist {
    typedef std::vector<AudioFile*>::iterator iterator;

  private:
    std::string             name;
    std::mutex              name_lock;
    std::vector<AudioFile*> playlist_member;
    std::mutex              playlist_member_lock;

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
    void        erase(iterator pos);
    void        clear();
    u64         size();
    bool        empty();
    AudioFile*  operator[](u64 n);
    Playlist() = default;
    ~Playlist();
};
} // namespace boxten