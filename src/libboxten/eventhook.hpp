#pragma once
#include <functional>

#include "playback.hpp"
#include "type.hpp"

namespace boxten{
enum Events {
    PLAYBACK_CHANGE,
    SONG_CHANGE,
};
using HookFunction = std::function<void(Events, void*)>;
namespace HookParameters {
struct PlaybackChange {
    PlaybackState old_state;
    PlaybackState new_state;
};
struct SongChange {
    i64 old_index;
    i64 new_index;
};
} // namespace Parameters
}