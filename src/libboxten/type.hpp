#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;

namespace boxten {
using n_frames = u64;

enum class SampleType : size_t {
    unknown = 0,
    f32_le,
    f32_be,
    s8,
    u8,
    s16_le,
    s16_be,
    u16_le,
    u16_be,
    s24_le,
    s24_be,
    u24_le,
    u24_be,
    s32_le,
    s32_be,
    u32_le,
    u32_be,
};
inline size_t get_sample_bytewidth(SampleType type) {
    switch(type) {
    case SampleType::s8:
    case SampleType::u8:
        return 1;
    case SampleType::s16_le:
    case SampleType::s16_be:
    case SampleType::u16_le:
    case SampleType::u16_be:
        return 2;
    case SampleType::s24_le:
    case SampleType::s24_be:
    case SampleType::u24_le:
    case SampleType::u24_be:
        return 3;
    case SampleType::f32_le:
    case SampleType::f32_be:
    case SampleType::s32_le:
    case SampleType::s32_be:
    case SampleType::u32_le:
    case SampleType::u32_be:
        return 4;
    default:
        return 0;
    }
}
struct PCMFormat {
    SampleType sample_type;
    u32        channels;
    u32        sampling_rate;
    size_t     get_sample_bytewidth() {
        return boxten::get_sample_bytewidth(sample_type);
    }
    bool operator==(const PCMFormat& a) const {
        return sample_type == a.sample_type &&
               channels == a.channels &&
               sampling_rate == a.sampling_rate;
    }
    bool operator!=(const PCMFormat& a) const {
        return !operator==(a);
    }
};
constexpr n_frames PCMPACKET_PERIOD = 512;
struct PCMPacketUnit {
    PCMFormat       format;
    u64             original_frame_pos[2];
    std::vector<u8> pcm;
    n_frames        get_frames() {
        return pcm.size() / get_sample_bytewidth(format.sample_type) / format.channels;
    }
};
using PCMPacket     = std::vector<PCMPacketUnit>;
using ComponentName = std::array<std::string, 2>;
using AudioTag      = std::map<std::string, std::string>;
struct LayoutData {
    enum {
        UNKNOWN,
        H_SPLIT,
        V_SPLIT,
        WIDGET
    } type;
    boxten::ComponentName   name;
    std::vector<LayoutData> children;
};
template <typename T>
struct SafeVar {
    T          data;
    std::mutex lock;
               operator std::mutex() {
        return lock;
    }
    operator T&() {
        return data;
    }
    T operator=(const T d) {
        data = d;
        return data;
    }
    T* operator->() {
        return &data;
    }
    SafeVar() {}
    SafeVar(T data) : data(data) {}
    SafeVar(const SafeVar<T>& src) {
        data = src.data;
    }
};
} // namespace boxten
