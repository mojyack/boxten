#pragma once
#include <mutex>

#include "audiofile.hpp"

namespace boxten {
class Playlist {
    typedef std::vector<AudioFile*>::iterator iterator;

  private:
    std::vector<AudioFile*> playlist_member;
    std::mutex              playlist_member_lock;

    void proc_insert(std::filesystem::path path, iterator pos);

  public:
    void        activate();
    std::mutex& mutex(); // lock this before call following functions
    iterator    begin();
    iterator    end();
    void        add(std::filesystem::path path);
    void        insert(std::filesystem::path path, iterator pos);
    void        erase(iterator pos);
    void        clear();
    u64         size();
    AudioFile*  operator[](u64 n);
    Playlist() = default;
    ~Playlist();
};
} // namespace boxten