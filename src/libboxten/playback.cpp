#include <chrono>
#include <fcntl.h>
#include <mutex>

#include "buffer.hpp"
#include "console.hpp"
#include "debug.hpp"
#include "eventhook_internal.hpp"
#include "playback.hpp"
#include "playback_internal.hpp"
#include "plugin.hpp"
#include "type.hpp"
#include "worker.hpp"

namespace boxten {
namespace {
StreamInput*                          stream_input  = nullptr;
StreamOutput*                         stream_output = nullptr;
SafeVar<std::vector<SoundProcessor*>> dsp_chain;
Playlist*                             playing_playlist = nullptr;

Buffer buffer;
bool   playback_starting = false; // if true, playback is started but stream_output->start_playback() has not called yet.
bool   playback_frozen   = false; // if true, stream_output->pause_playback() was called in order to wait buffer filled after underrun.
void   buffer_underrun_handler() {
    DEBUG_OUT("buffer underrun!");
    if(!get_if_playlist_left()) {
        stop_playback();
    } else if(!playback_frozen) {
        stream_output->pause_playback();
        playback_frozen = true;
    }
}

struct FilledFramePos {
    i64 song = -1;
    u64 frame;
};
SafeVar<FilledFramePos> filled_frame_pos;
bool                    end_of_playlist           = false; // If true, all of playlist were sent to buffer already.
bool                    finish_fill_buffer_thread = false;
void                    fill_buffer() {
    while(1) {
        if(playback_starting || playback_frozen) {
            if(buffer.has_enough_packets()) {
                if(playback_starting) {
                    stream_output->start_playback();
                    playback_starting = false;
                } else if(playback_frozen) {
                    stream_output->resume_playback();
                    playback_frozen = false;
                }
            }
        }

        std::unique_lock<std::mutex> lock(buffer.need_fill_buffer.lock);
        buffer.continue_fill_buffer.wait(lock, [&] {
            return buffer.need_fill_buffer || finish_fill_buffer_thread;
        });
        if(finish_fill_buffer_thread) break;
        while(buffer.free_frame() >= PCMPACKET_PERIOD && !end_of_playlist) {
            LOCK_GUARD_D(filled_frame_pos.lock, lock);
            LOCK_GUARD_D(playing_playlist->mutex(), plock);
            if(filled_frame_pos->song >= static_cast<i64>(playing_playlist->size())) {
                end_of_playlist = true;
                continue;
            }
            auto&    audio_file  = *(*playing_playlist)[filled_frame_pos->song];
            n_frames frames_left = audio_file.get_total_frames() - (filled_frame_pos->frame + 1);
            n_frames to_read     = frames_left >= PCMPACKET_PERIOD ? PCMPACKET_PERIOD : frames_left;
            auto     packet      = stream_input->read_frames(audio_file, filled_frame_pos->frame, to_read);
            {
                LOCK_GUARD_D(dsp_chain.lock, lock);
                for(auto c : dsp_chain.data) {
                    c->modify_packet(packet);
                }
            }
            filled_frame_pos->frame = packet.original_frame_pos[1] + 1;
            if(filled_frame_pos->frame >= audio_file.get_total_frames()) {
                if(filled_frame_pos->song + 1 == static_cast<i64>(playing_playlist->size())) {
                    end_of_playlist = true;
                    continue;
                } else {
                    invoke_eventhook(Events::SONG_CHANGE, new HookParameters::SongChange{static_cast<i64>(filled_frame_pos->song), static_cast<i64>(filled_frame_pos->song + 1)});
                    filled_frame_pos->song++;
                    filled_frame_pos->frame = 0;
                }
            }
            buffer.append(packet);
        }
        buffer.need_fill_buffer = false;
    }
}
Worker fill_buffer_thread;

struct PlayingPacket {
    struct PacketFormat {
        u64 playing_frame_pos[2];
        u32 sampling_rate;
    };
    std::optional<u64>                    seeked_to; // if holds value, use this as a playing pos.
    std::vector<PacketFormat>             packet_formats;
    std::chrono::system_clock::time_point update_time;
    void                                  refresh(const PCMPacket& packet) {
        packet_formats.clear();
        for(auto& p : packet) {
            PlayingPacket::PacketFormat format;
            format.playing_frame_pos[0] = p.original_frame_pos[0];
            format.playing_frame_pos[1] = p.original_frame_pos[1];
            format.sampling_rate        = p.format.sampling_rate;
            packet_formats.emplace_back(format);
        }
        update_time = std::chrono::system_clock::now();
    };
};
SafeVar<PlayingPacket> playing_packet;
u64                    paused_frame_pos = 0;

PlaybackState playback_state = PlaybackState::STOPPED;

enum COMMAND {
    PLAY,
    STOP,
    PAUSE,
    RESUME,
    SEEK_RATE_ABS,
    SEEK_RATE_REL,
    CHANGE_SONG_ABS,
    CHANGE_SONG_REL,
};
void proc_resume_playback();
i64  proc_get_playing_index() {
    if(playback_state == PlaybackState::STOPPED) return -1;
    LOCK_GUARD_D(filled_frame_pos.lock, lock);
    return filled_frame_pos->song;
}
i64 proc_get_playback_pos() {
    static u64 fallback = 0;
    if(playback_state == PlaybackState::STOPPED) return -1;
    if(playback_state == PlaybackState::PAUSED) return paused_frame_pos;
    LOCK_GUARD_D(playing_packet.lock, pplock);
    if(playing_packet->seeked_to) {
        return playing_packet->seeked_to.value();
    }
    auto now = std::chrono::system_clock::now();
    for(auto& p : playing_packet->packet_formats) {
        auto     elapsed     = std::chrono::duration_cast<std::chrono::milliseconds>(now - playing_packet->update_time).count();
        n_frames dur         = p.playing_frame_pos[1] - p.playing_frame_pos[0];
        u64      miliseconds = 1000.0 * dur / p.sampling_rate;
        if(static_cast<u64>(elapsed) > miliseconds) {
            continue;
        }
        f64 rate    = static_cast<f64>(elapsed) / static_cast<f64>(miliseconds);
        u64 current = p.playing_frame_pos[0] + dur * rate;
        u64 delay   = stream_output->output_delay();
        fallback    = current < delay ? 0 : current - delay;
        return fallback;
    }
    // Sometimes reach here because of timer error.
    // Simply return previous result.
    return fallback;
}
void proc_start_playback() {
    if(playing_playlist == nullptr) return;
    if(playback_state == PlaybackState::PLAYING) return;
    if(playback_state == PlaybackState::PAUSED) {
        proc_resume_playback();
        return;
    }

    if(LOCK_GUARD_D(playing_playlist->mutex(), plock); playing_playlist->empty()) return;

    LOCK_GUARD_D(filled_frame_pos.lock, lock);
    invoke_eventhook(Events::SONG_CHANGE, new HookParameters::SongChange{filled_frame_pos->song, 0});
    filled_frame_pos->song  = 0;
    filled_frame_pos->frame = 0;

    end_of_playlist = false;
    buffer.set_buffer_underrun_handler(buffer_underrun_handler);

    finish_fill_buffer_thread = false;
    fill_buffer_thread        = Worker(fill_buffer);

    playback_starting = true;
    invoke_eventhook(Events::PLAYBACK_CHANGE, new HookParameters::PlaybackChange{playback_state, PlaybackState::PLAYING});
    playback_state = PlaybackState::PLAYING;
}
void proc_stop_playback() {
    if(playback_state == PlaybackState::STOPPED) return;

    finish_fill_buffer_thread = true;
    buffer.continue_fill_buffer.notify_one();
    fill_buffer_thread.join();

    stream_output->stop_playback();
    invoke_eventhook(Events::PLAYBACK_CHANGE, new HookParameters::PlaybackChange{playback_state, PlaybackState::STOPPED});
    playback_state = PlaybackState::STOPPED;

    buffer.clear();
}
void proc_pause_playback() {
    if(playback_state == PlaybackState::PAUSED) return;
    if(playback_state == PlaybackState::STOPPED) return;
    paused_frame_pos = proc_get_playback_pos();
    stream_output->pause_playback();
    invoke_eventhook(Events::PLAYBACK_CHANGE, new HookParameters::PlaybackChange{playback_state, PlaybackState::PAUSED});
    playback_state = PlaybackState::PAUSED;
}
void proc_resume_playback() {
    if(playback_state != PlaybackState::PAUSED) return;

    {
        LOCK_GUARD_D(playing_packet.lock, lock);
        playing_packet->update_time = std::chrono::system_clock::now();
    }
    stream_output->resume_playback();
    invoke_eventhook(Events::PLAYBACK_CHANGE, new HookParameters::PlaybackChange{playback_state, PlaybackState::PLAYING});
    playback_state = PlaybackState::PLAYING;
}
void proc_seek_rate_abs(f64 rate) {
    if(rate < 0.0) return;
    if(rate > 1.0) return;
    {
        LOCK_GUARD_D(filled_frame_pos.lock, lock);
        LOCK_GUARD_D(playing_playlist->mutex(), plock);

        if(filled_frame_pos->song < 0 || filled_frame_pos->song >= static_cast<i64>(playing_playlist->size())) return;
        auto& audio_file        = *(*playing_playlist)[filled_frame_pos->song];
        filled_frame_pos->frame = audio_file.get_total_frames() * rate;

        LOCK_GUARD_D(playing_packet.lock, pplock);
        playing_packet->seeked_to = filled_frame_pos->frame;
    }
    buffer.clear();
}
void proc_seek_rate_rel(f64 rate) {
    if(rate < -1.0) return;
    if(rate > 1.0) return;
    {
        LOCK_GUARD_D(filled_frame_pos.lock, lock);
        LOCK_GUARD_D(playing_playlist->mutex(), plock);

        if(filled_frame_pos->song < 0 || filled_frame_pos->song >= static_cast<i64>(playing_playlist->size())) return;
        auto& audio_file = *(*playing_playlist)[filled_frame_pos->song];
        filled_frame_pos->frame *= rate;
        if(filled_frame_pos->frame + 1 >= audio_file.get_total_frames()) {
            if(filled_frame_pos->song == static_cast<i64>(playing_playlist->size())) {
                end_of_playlist = true;
            } else {
                filled_frame_pos->song++;
                filled_frame_pos->frame = 0;
            }
        }
        LOCK_GUARD_D(playing_packet.lock, pplock);
        playing_packet->seeked_to = filled_frame_pos->frame;
    }
    buffer.clear();
}
void proc_change_song_abs(i64 index) {
    if(index < 0) return;
    {
        LOCK_GUARD_D(playing_playlist->mutex(), plock);
        if(index >= static_cast<i64>(playing_playlist->size())) return;
        LOCK_GUARD_D(filled_frame_pos.lock, flock);
        invoke_eventhook(Events::SONG_CHANGE, new HookParameters::SongChange{filled_frame_pos->song, index});
        filled_frame_pos->song  = index;
        filled_frame_pos->frame = 0;

        LOCK_GUARD_D(playing_packet.lock, pplock);
        playing_packet->seeked_to = filled_frame_pos->frame;
    }
    buffer.clear();
}
void proc_change_song_rel(i64 val) {
    if(val == 0) return;
    {
        LOCK_GUARD_D(filled_frame_pos.lock, flock);
        LOCK_GUARD_D(playing_playlist->mutex(), plock);
        if(filled_frame_pos->song + val < 0) return;
        if(filled_frame_pos->song + val >= static_cast<i64>(playing_playlist->size())) return;
        invoke_eventhook(Events::SONG_CHANGE, new HookParameters::SongChange{filled_frame_pos->song, filled_frame_pos->song + val});
        filled_frame_pos->song += val;
        filled_frame_pos->frame = 0;

        LOCK_GUARD_D(playing_packet.lock, pplock);
        playing_packet->seeked_to = filled_frame_pos->frame;
    }
    buffer.clear();
}

struct PlaybackControl {
    COMMAND command;
    i64     arg;
};
class PlaybackThread : public QueueThread<PlaybackControl> {
  private:
    void proc(std::vector<PlaybackControl> queue_to_proc) override {
        for(auto c : queue_to_proc) {
            switch(c.command) {
            case COMMAND::PLAY:
                proc_start_playback();
                break;
            case COMMAND::STOP:
                proc_stop_playback();
                break;
            case COMMAND::PAUSE:
                proc_pause_playback();
                break;
            case COMMAND::RESUME:
                proc_resume_playback();
                break;
            case COMMAND::SEEK_RATE_ABS:
                proc_seek_rate_abs(*reinterpret_cast<f64*>(&c.arg));
                break;
            case COMMAND::SEEK_RATE_REL:
                proc_seek_rate_rel(*reinterpret_cast<f64*>(&c.arg));
                break;
            case COMMAND::CHANGE_SONG_ABS:
                proc_change_song_abs(c.arg);
                break;
            case COMMAND::CHANGE_SONG_REL:
                proc_change_song_rel(c.arg);
                break;
            }
        }
    }

  public:
    ~PlaybackThread() {}
};
PlaybackThread playback_thread;
} // namespace

void start_playback(bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::PLAY, 0});
    if(blocking) playback_thread.wait_empty();
}
void stop_playback(bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::STOP, 0});
    if(blocking) playback_thread.wait_empty();
}
void pause_playback(bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::PAUSE, 0});
    if(blocking) playback_thread.wait_empty();
}
void resume_playback(bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::RESUME, 0});
    if(blocking) playback_thread.wait_empty();
}
void seek_rate_abs(f64 rate, bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::SEEK_RATE_ABS, *reinterpret_cast<i64*>(&rate)});
    if(blocking) playback_thread.wait_empty();
}
void seek_rate_rel(f64 rate, bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::SEEK_RATE_REL, *reinterpret_cast<i64*>(&rate)});
    if(blocking) playback_thread.wait_empty();
}
void change_song_abs(i64 index, bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::CHANGE_SONG_ABS, index});
    if(blocking) playback_thread.wait_empty();
}
void change_song_rel(i64 val, bool blocking) {
    playback_thread.enqueue(PlaybackControl{COMMAND::CHANGE_SONG_REL, val});
    if(blocking) playback_thread.wait_empty();
}
i64 get_playing_index() {
    LOCK_GUARD_D(playback_thread.wait_empty(), lock);
    return proc_get_playing_index();
}
i64 get_playback_pos() {
    LOCK_GUARD_D(playback_thread.wait_empty(), lock);
    return proc_get_playback_pos();
}
PlaybackState get_playback_state() {
    LOCK_GUARD_D(playback_thread.wait_empty(), lock);
    return playback_state;
}
n_frames get_playing_song_length() {
    LOCK_GUARD_D(playback_thread.wait_empty(), qlock);
    LOCK_GUARD_D(filled_frame_pos.lock, flock);
    LOCK_GUARD_D(playing_playlist->mutex(), plock);

    if(filled_frame_pos->song < 0 || filled_frame_pos->song >= static_cast<i64>(playing_playlist->size())) return 0;
    auto& audio_file = *(*playing_playlist)[filled_frame_pos->song];
    return audio_file.get_total_frames();
}
bool get_if_playlist_left() {
    return !end_of_playlist;
}

/* internal */
void set_stream_input(StreamInput* input) {
    stream_input = input;
}
void set_stream_output(StreamOutput* output) {
    stream_output = output;
}
void set_dsp_chain(std::vector<SoundProcessor*> dsp) {
    LOCK_GUARD_D(dsp_chain.lock, lock);
    dsp_chain.data = dsp;
}
void start_playback_thread() {
    playback_thread.start();
}
void finish_playback_thread() {
    playback_thread.finish();
}

void set_playlist(Playlist* playlist) {
    stop_playback(true);
    playing_playlist = playlist;
}
void unset_playlist() {
    stop_playback(true);
    playing_playlist = nullptr;
}
void playing_playlist_insert(u64 pos, AudioFile* audio_file) {
    // playing_playlist->mutex() must be locked
    // Because this function only be called from Playlist::proc_insert()
    LOCK_GUARD_D(filled_frame_pos.lock, lock);

    if(static_cast<i64>(pos) > filled_frame_pos->song) {
        /* New music will be inserted after playing music. Nothing to do. */
    } else {
        /* correction filled_frame_pos */
        filled_frame_pos->song++;
    }
    return;
}
void playing_playlist_erase(u64 pos) {
    // playing_playlist->mutex() must be locked
    // Because this function only be called from Playlist::erase()
    LOCK_GUARD_D(filled_frame_pos.lock, lock);

    if(static_cast<i64>(pos) > filled_frame_pos->song) {
        /* The music is after playing music. Nothing to do. */
    } else if(static_cast<i64>(pos) < filled_frame_pos->song) {
        filled_frame_pos->song--;
    } else { // pos == playing_music_num
        filled_frame_pos->frame = 0;
    }
    if(playing_playlist->empty()) stop_playback();
    return;
}

n_frames get_buffer_filled_frames() {
    return buffer.filled_frame();
}
PCMPacket get_buffer_pcm_packet(n_frames frames) {
    auto packet = buffer.cut(frames);
    if(!packet.empty()) {
        LOCK_GUARD_D(playing_packet.lock, lock);
        playing_packet->refresh(packet);
        playing_packet->seeked_to.reset();
    }
    return packet;
}
PCMFormat get_buffer_pcm_format() {
    return buffer.get_next_format();
}

n_frames get_total_frames(AudioFile* audio_file) {
    return stream_input->calc_total_frames(*audio_file);
}
AudioTag get_tags(AudioFile* audio_file) {
    return stream_input->read_tags(*audio_file);
}
} // namespace boxten
