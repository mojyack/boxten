/* This is an internal header, which will not be installed. */
#include "playlist.hpp"
#include "plugin.hpp"
#include "queuethread.hpp"

namespace boxten{
void      set_stream_input(StreamInput* input);
void      set_stream_output(StreamOutput* output);
void      set_playlist(Playlist* playlist);
void      unset_playlist();
void      start_playback_thread();
void      finish_playback_thread();
n_frames  get_buffer_filled_frames();
PCMPacket get_buffer_pcm_packet(n_frames frames);
PCMFormat get_buffer_pcm_format();

// Notice playback thread to correct filled_buffer_frame before insert/erase music to playlist.
// This function returns locked filled_buffer_frame_lock, insert/erase music and unlock it immediately
std::mutex& playing_playlist_insert(u64 pos, AudioFile* audio_file);
std::mutex& playing_playlist_erase(u64 pos);

} // namespace boxten