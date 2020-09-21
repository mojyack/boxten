#include <mutex>

#include "debug.hpp"
#include "playback_internal.hpp"
#include "playlist.hpp"

namespace boxten{
namespace {
class AudioFileManager{
  private:
    struct AudioFilePointer {
        AudioFile* audio_file;
        u32        ref_count = 0;
        AudioFilePointer(std::filesystem::path path){
            audio_file = new AudioFile(path);
        }
        ~AudioFilePointer(){
            if(ref_count == 0) delete audio_file;
        }
    };
    std::vector<AudioFilePointer> audio_files;
  public:
    AudioFile* get_audio_ref(std::filesystem::path path){
        std::vector<AudioFilePointer>::iterator audio_file_pointer = audio_files.end();
        for(auto a = audio_files.begin(); a != audio_files.end(); ++a) {
            if(a->audio_file->get_path() == path) {
                audio_file_pointer = a;
                break;
            }
        }
        if(audio_file_pointer == audio_files.end()){
            audio_files.emplace_back(path);
            audio_file_pointer = audio_files.end() - 1;
        }
        audio_file_pointer->ref_count++;
        return audio_file_pointer->audio_file;
    }
    void release_audio_ref(AudioFile* audio_file){
        for(auto a = audio_files.begin(); a != audio_files.end(); ++a) {
            if(a->audio_file == audio_file){
                a->ref_count--;
                if(a->ref_count == 0){
                    audio_files.erase(a);
                }
                return;
            }
        }
        DEBUG_OUT("audio file pointer not found.");
    }
};
AudioFileManager audio_files;
std::mutex       audio_files_lock;

Playlist*  playing_playlist = nullptr;
std::mutex playing_playlist_lock;
} // namespace

void Playlist::proc_insert(std::filesystem::path path, iterator pos){
    LOCK_GUARD_D(audio_files_lock, Playlist::proc_insert, aflock);

    auto audio_file_ref = audio_files.get_audio_ref(path);
    LOCK_GUARD_D(playing_playlist_lock, Playlist::proc_insert, pplock);
    if(playing_playlist == this) {
        auto& lock = playing_playlist_insert(std::distance(begin(), pos), audio_file_ref);
        playlist_member.emplace_back(audio_file_ref);
        lock.unlock();
    } else {
        playlist_member.emplace_back(audio_file_ref);
    }
}
void Playlist::activate() {
    boxten::set_playlist(this);
    LOCK_GUARD_D(playing_playlist_lock, Playlist::set_playing, lock);
    playing_playlist = this;
}
std::mutex& Playlist::mutex() {
    return playlist_member_lock;
}
Playlist::iterator Playlist::begin() {
    return playlist_member.begin();
}
Playlist::iterator Playlist::end() {
    return playlist_member.end();
}
void Playlist::add(std::filesystem::path path) {
    proc_insert(path, playlist_member.end());
}
void Playlist::insert(std::filesystem::path path, iterator pos) {
    proc_insert(path, pos);
}
void Playlist::erase(iterator pos){
    auto to_erase_audio = *pos;
    LOCK_GUARD_D(playing_playlist_lock, Playlist::proc_insert, pplock);
    if(playing_playlist == this) {
        auto& lock = playing_playlist_erase(std::distance(begin(), pos));
        playlist_member.erase(pos);
        lock.unlock();
    } else {
        playlist_member.erase(pos);
    }
    audio_files.release_audio_ref(to_erase_audio);
}
void Playlist::clear() {
    for(auto a = playlist_member.begin(); a != playlist_member.end();++a) {
        erase(a);
    }
}
u64 Playlist::size() {
    return playlist_member.size();
}
AudioFile* Playlist::operator[](u64 n) {
    return playlist_member[n];
}
Playlist::~Playlist() {
    {
        LOCK_GUARD_D(playing_playlist_lock, Playlist::set_playing, pplock);
        if(playing_playlist == this) {
            DEBUG_OUT("Deleting playing playlist!");
        }
    }
    LOCK_GUARD_D(playlist_member_lock, Playlist::~Playlist, pmlock);
    clear();
}
}