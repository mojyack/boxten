/* This is an internal header, which will not be installed. */
#pragma once
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <functional>

#include "type.hpp"

namespace boxten {
class Buffer {
  private:
    SafeVar<PCMPacket>        data;
    std::function<void(void)> buffer_underrun_handler;

    void notify_need_fill_buffer();

  public:
    SafeVar<bool>           need_fill_buffer = true;
    std::condition_variable continue_fill_buffer;

    void clear();
    n_frames filled_frame();
    n_frames free_frame();
    void append(PCMPacketUnit& packet);
    void append(PCMPacket& packet);
    PCMPacket cut(n_frames frame);
    PCMFormat get_next_format();
    bool      has_enough_packets();

    void set_buffer_underrun_handler(std::function<void(void)> handler);
};
} // namespace boxten