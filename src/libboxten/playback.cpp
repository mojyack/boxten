#include <chrono>
#include <mutex>

#include "buffer.hpp"
#include "console.hpp"
#include "eventhook_internal.hpp"
#include "playback.hpp"
#include "playback_internal.hpp"
#include "worker.hpp"

namespace boxten {
namespace {
StreamInput*  stream_input     = nullptr;
StreamOutput* stream_output    = nullptr;
Playlist*     playing_playlist = nullptr;

Buffer     buffer;
struct {
    u64 song;
    u64 frame;
} filled_frame_pos;
std::mutex filled_frame_pos_lock;
bool       end_of_playlist     = false; // If true, all of playlist were sent to buffer already.
bool       finish_fill_buffer_thread = false;
void       fill_buffer() {
    while(1) {
        std::unique_lock<std::mutex> lock(buffer.need_fill_buffer_lock);
        buffer.continue_fill_buffer.wait(lock, [&] {
            return buffer.need_fill_buffer || finish_fill_buffer_thread;
        });
        if(finish_fill_buffer_thread) break;
        while(buffer.free_frame() >= PCMPACKET_PERIOD && !end_of_playlist) {
            LOCK_GUARD_D(filled_frame_pos_lock, lock);
            LOCK_GUARD_D(playing_playlist->mutex(), plock);
            if(filled_frame_pos.song >= playing_playlist->size()){
                end_of_playlist = true;
                continue;
            }
            auto& audio_file = *(*playing_playlist)[filled_frame_pos.song];
            n_frames frames_left = audio_file.get_total_frames() - (filled_frame_pos.frame + 1);
            n_frames to_read     = frames_left >= PCMPACKET_PERIOD ? PCMPACKET_PERIOD : frames_left;
            auto     packet      = stream_input->read_frames(audio_file, filled_frame_pos.frame, to_read);
            filled_frame_pos.frame += packet.get_frames();
            if(filled_frame_pos.frame + 1 >= audio_file.get_total_frames()) {
                if(filled_frame_pos.song + 1 == playing_playlist->size()){
                    end_of_playlist = true;
                    continue;
                } else {
                    filled_frame_pos.song++;
                    filled_frame_pos.frame = 0;
                    // Considering the delay caused by the buffer, SONG_CHANGE hook is not invoked here.
                }
            }
            buffer.append(packet);
        }
        buffer.need_fill_buffer = false;
    }
}
Worker     fill_buffer_thread;
void       buffer_underrun_handler() {
    printf("underrun!");
    if(!get_if_playlist_left()) stop_playback();
}

struct PacketFormat {
    u64 playing_frame_pos[2];
    u32 sampling_rate;
};
std::vector<PacketFormat>             playing_packet;
std::chrono::system_clock::time_point playing_packet_updated_time;
std::mutex                            playing_packet_lock;
u64                                   paused_frame_pos = 0;

PLAYBACK_STATE playback_state = PLAYBACK_STATE::STOPPED;

enum COMMAND {
    PLAY,
    STOP,
    PAUSE,
    RESUME,
    SEEK_RATE_ABS,
    SEEK_RATE_REL,
};
void proc_resume_playback();
u64 proc_get_playback_pos() {
    static u64 fallback = 0;
    if(playback_state == PLAYBACK_STATE::STOPPED) return 0;
    if(playback_state == PLAYBACK_STATE::PAUSED) return paused_frame_pos;
    LOCK_GUARD_D(playing_packet_lock, pplock);
    auto now     = std::chrono::system_clock::now();
    for(auto& p : playing_packet) {
        auto     elapsed     = std::chrono::duration_cast<std::chrono::milliseconds>(now - playing_packet_updated_time).count();
        n_frames dur         = p.playing_frame_pos[1] - p.playing_frame_pos[0];
        u64      miliseconds = 1000.0 * dur / p.sampling_rate;
        if(static_cast<u64>(elapsed) > miliseconds) {
            continue;
        }
        f64 rate    = static_cast<f64>(elapsed) / static_cast<f64>(miliseconds);
        u64 current = p.playing_frame_pos[0] + dur * rate;
        u64 delay   = stream_output->output_delay();
        fallback    = current < delay ? 0 : current - delay;
        DEBUG_OUT(fallback);
        return fallback;
    }
    // Sometimes reach here because of timer error.
    // Simply return previous result.
    return fallback;
}
void proc_start_playback() {
    if(playing_playlist == nullptr) return;
    if(playback_state == PLAYBACK_STATE::PLAYING) return;
    if(playback_state == PLAYBACK_STATE::PAUSED) {
        proc_resume_playback();
        return;
    }

    if(LOCK_GUARD_D(playing_playlist->mutex(), plock); playing_playlist->empty()) return;

    LOCK_GUARD_D(filled_frame_pos_lock, lock);
    filled_frame_pos.song = 0;
    filled_frame_pos.frame = 0;

    end_of_playlist = false;
    buffer.set_buffer_underrun_handler(buffer_underrun_handler);

    finish_fill_buffer_thread = false;
    fill_buffer_thread  = Worker(fill_buffer);

    stream_output->start_playback();
    playback_state = PLAYBACK_STATE::PLAYING;

    invoke_eventhook(EVENT::PLAYBACK_START);
}
void proc_stop_playback() {
    if(playback_state == PLAYBACK_STATE::STOPPED) return;

    finish_fill_buffer_thread = true;
    buffer.continue_fill_buffer.notify_one();
    fill_buffer_thread.join();

    stream_output->stop_playback();
    playback_state = PLAYBACK_STATE::STOPPED;
    invoke_eventhook(EVENT::PLAYBACK_STOP);

    buffer.clear();
}
void proc_pause_playback() {
    if(playback_state == PLAYBACK_STATE::PAUSED) return;
    if(playback_state == PLAYBACK_STATE::STOPPED) return;
    paused_frame_pos = proc_get_playback_pos();
    stream_output->pause_playback();
    playback_state = PLAYBACK_STATE::PAUSED;
    invoke_eventhook(EVENT::PLAYBACK_PAUSE);
}
void proc_resume_playback() {
    if(playback_state != PLAYBACK_STATE::PAUSED) return;

    {
        LOCK_GUARD_D(playing_packet_lock, lock);
        playing_packet_updated_time = std::chrono::system_clock::now();
    }
    stream_output->resume_playback();
    playback_state = PLAYBACK_STATE::PLAYING;
    invoke_eventhook(EVENT::PLAYBACK_RESUME);
}
void proc_seek_rate_abs(f64 rate) {
    if(rate < 0.0) return;
    if(rate > 1.0) return;
    LOCK_GUARD_D(filled_frame_pos_lock, lock);
    LOCK_GUARD_D(playing_playlist->mutex(), plock);

    if(filled_frame_pos.song >= playing_playlist->size()) return;
    auto& audio_file = *(*playing_playlist)[filled_frame_pos.song];
    filled_frame_pos.frame = audio_file.get_total_frames() * rate;
    buffer.clear();
}
void proc_seek_rate_rel(f64 rate) {
    if(rate < -1.0) return;
    if(rate > 1.0) return;
    LOCK_GUARD_D(filled_frame_pos_lock, lock);
    LOCK_GUARD_D(playing_playlist->mutex(), plock);

    if(filled_frame_pos.song >= playing_playlist->size()) return;
    auto& audio_file = *(*playing_playlist)[filled_frame_pos.song];
    filled_frame_pos.frame *= rate;
    if(filled_frame_pos.frame + 1 >= audio_file.get_total_frames()) {
        if(filled_frame_pos.song == playing_playlist->size()) {
            end_of_playlist = true;
        } else {
            filled_frame_pos.song++;
            filled_frame_pos.frame = 0;
        }
    }
    buffer.clear();
}

struct PlaybackControl {
    COMMAND command;
    u64     arg;
    PlaybackControl(COMMAND command, u64 arg = 0) : command(command), arg(arg) {}
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
            }
        }
    }

  public:
    ~PlaybackThread() {}
};
PlaybackThread playback_thread;
} // namespace

void start_playback(bool blocking) {
    playback_thread.enqueue(PlaybackControl(COMMAND::PLAY, 0));
    if(blocking) playback_thread.wait_empty();
}
void stop_playback(bool blocking){
    playback_thread.enqueue(PlaybackControl(COMMAND::STOP, 0));
    if(blocking) playback_thread.wait_empty();
}
void pause_playback(bool blocking){
    playback_thread.enqueue(PlaybackControl(COMMAND::PAUSE, 0));
    if(blocking) playback_thread.wait_empty();
}
void resume_playback(bool blocking){
    playback_thread.enqueue(PlaybackControl(COMMAND::RESUME, 0));
    if(blocking) playback_thread.wait_empty();
}
void seek_rate_abs(f64 rate, bool blocking) {
    playback_thread.enqueue(PlaybackControl(COMMAND::SEEK_RATE_ABS, *reinterpret_cast<u64*>(&rate)));
    if(blocking) playback_thread.wait_empty();
}
void seek_rate_rel(f64 rate, bool blocking){
    playback_thread.enqueue(PlaybackControl(COMMAND::SEEK_RATE_REL, *reinterpret_cast<u64*>(&rate)));
    if(blocking) playback_thread.wait_empty();
}
u64 get_playback_pos() {
    LOCK_GUARD_D(playback_thread.wait_empty(), lock);
    return proc_get_playback_pos();
}
PLAYBACK_STATE get_playback_state(){
    LOCK_GUARD_D(playback_thread.wait_empty(), lock);
    return playback_state;
}
n_frames get_playing_song_length() {
    LOCK_GUARD_D(playback_thread.wait_empty(), qlock);
    LOCK_GUARD_D(filled_frame_pos_lock, flock);
    LOCK_GUARD_D(playing_playlist->mutex(), plock);

    if(filled_frame_pos.song >= playing_playlist->size()) return 0;
    auto& audio_file = *(*playing_playlist)[filled_frame_pos.song];
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
    LOCK_GUARD_D(filled_frame_pos_lock, lock);

    if(pos > filled_frame_pos.song) {
        /* New music will be inserted after playing music. Nothing to do. */
    } else {
        /* correction filled_frame_pos */
        filled_frame_pos.song++;
    }
    return;
}
void playing_playlist_erase(u64 pos) {
    // playing_playlist->mutex() must be locked
    // Because this function only be called from Playlist::erase()
    LOCK_GUARD_D(filled_frame_pos_lock, lock);

    if(pos > filled_frame_pos.song) {
        /* The music is after playing music. Nothing to do. */
    } else if(pos < filled_frame_pos.song) {
        filled_frame_pos.song--;
    } else { // pos == playing_music_num
        filled_frame_pos.frame = 0;
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
        LOCK_GUARD_D(playing_packet_lock, lock);
        playing_packet.clear();
        for(auto& p : packet) {
            PacketFormat format;
            format.playing_frame_pos[0] = p.original_frame_pos[0];
            format.playing_frame_pos[1] = p.original_frame_pos[1];
            format.sampling_rate        = p.format.sampling_rate;
            playing_packet.emplace_back(format);
        }
        for(auto i = playing_packet.begin() + 1; i != playing_packet.end();++i){
            if(i->playing_frame_pos[0] < (i - 1)->playing_frame_pos[1]){
                invoke_eventhook(EVENT::SONG_CHANGE);
                break;
            }
        }
        playing_packet_updated_time = std::chrono::system_clock::now();
    }
    return packet;
}
PCMFormat get_buffer_pcm_format(){
    return buffer.get_next_format();
}

n_frames get_total_frames(AudioFile* audio_file){
    return stream_input->calc_total_frames(*audio_file);
}
AudioTag get_tags(AudioFile* audio_file){
    return stream_input->read_tags(*audio_file);
}
} // namespace boxten
