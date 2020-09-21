#pragma once
#include "type.hpp"

namespace boxten {
class Playlist;
enum PLAYBACK_STATE {
    STOPPED,
    PLAYING,
    PAUSED,
};

void           start_playback(bool blocking = false);
void           stop_playback(bool blocking = false);
void           pause_playback(bool blocking = false);
void           resume_playback(bool blocking = false);
void           seek_rate_abs(f64 rate, bool blocking = false);
void           seek_rate_rel(f64 rate, bool blocking = false);
u64            get_playback_pos();
PLAYBACK_STATE get_playback_state();
n_frames       get_playing_song_length();
bool           get_if_playlist_left();
} // namespace boxten