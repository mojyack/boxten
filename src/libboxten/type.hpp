#pragma once
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

typedef std::int8_t i8;
typedef std::int16_t i16;
typedef std::int32_t i32;
typedef std::int64_t i64;
typedef std::uint8_t u8;
typedef std::uint16_t u16;
typedef std::uint32_t u32;
typedef std::uint64_t u64;
typedef float f32;
typedef double f64;

namespace boxten {
typedef u64 n_frames;

enum FORMAT_SAMPLE_TYPE {
    UNKNOWN,
    FLOAT,
    S8,
    U8,
    S16_LE,
    S16_BE,
    U16_LE,
    U16_BE,
    S24_LE,
    S24_BE,
    U24_LE,
    U24_BE,
    S32_LE,
    S32_BE,
    U32_LE,
    U32_BE,
};
struct PCMFormat {
    FORMAT_SAMPLE_TYPE sample_type;
    u32 channels;
    u32 sampling_rate;
    u32 get_bytewidth() {
        switch(sample_type) {
        case S8:
        case U8:
            return 1;
        case S16_LE:
        case S16_BE:
        case U16_LE:
        case U16_BE:
            return 2;
        case S24_LE:
        case S24_BE:
        case U24_LE:
        case U24_BE:
            return 3;
        case FLOAT:
        case S32_LE:
        case S32_BE:
        case U32_LE:
        case U32_BE:
            return 4;
        default:
            return 0;
        }
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
    PCMFormat format;
    u64 original_frame_pos[2];
    std::vector<u8> pcm;
    n_frames get_frames() {
        return pcm.size() / format.get_bytewidth() / format.channels;
    }
};
typedef std::vector<PCMPacketUnit>         PCMPacket;
typedef std::array<std::string, 2>         ComponentName;
typedef std::map<std::string, std::string> AudioTag;
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
struct SafeVar{
    T data;
    std::mutex lock;
    operator std::mutex(){
        return lock;
    }
    operator T&(){
        return data;
    }
    T operator=(const T d){
        data = d;
        return data;
    }
    T* operator->(){
        return &data;
    }
    SafeVar(){}
    SafeVar(T data) : data(data) {}
    SafeVar(const SafeVar<T>& src){
        data = src.data;
    }
};
} // namespace boxten