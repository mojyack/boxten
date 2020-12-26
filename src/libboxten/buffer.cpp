#include "buffer.hpp"


namespace boxten{
constexpr u64 buffer_limit = PCMPACKET_PERIOD * 32; // frames
constexpr u64 unfreeze_threshold = PCMPACKET_PERIOD * 16;

void Buffer::notify_need_fill_buffer(){
    std::lock_guard<std::mutex> lock(need_fill_buffer.lock);
    need_fill_buffer = true;
    continue_fill_buffer.notify_one();
}
void Buffer::clear(){
    {
        std::lock_guard<std::mutex> lock(data.lock);
        data->clear();
    }
    notify_need_fill_buffer();
}
n_frames Buffer::filled_frame() {
    n_frames filled = 0;
    std::lock_guard<std::mutex> lock(data.lock);
    for(auto& b : static_cast<PCMPacket&>(data)) {
        filled += b.get_frames();
    }
    return filled;
}
n_frames Buffer::free_frame() {
    auto filled = filled_frame();
    return buffer_limit < filled ? 0 : buffer_limit - filled;
}
void Buffer::append(PCMPacketUnit& packet) {
    std::lock_guard<std::mutex> lock(data.lock);
    data->emplace_back();
    {
        auto& buff  = data->back();
        buff.format = packet.format;
        buff.original_frame_pos[0] = packet.original_frame_pos[0];
        buff.original_frame_pos[1] = packet.original_frame_pos[1];
        std::copy(packet.pcm.begin(), packet.pcm.end(), std::back_inserter(buff.pcm));
    }
}
void Buffer::append(PCMPacket& packet) {
    for(auto& unit : packet){
        append(unit);
    }
}
PCMPacket Buffer::cut(n_frames frame) {
    {
        auto filled = filled_frame();
        if(frame > filled) {
            if(buffer_underrun_handler) buffer_underrun_handler();
            frame = filled;
        }
    }

    auto to_cut = frame;
    PCMPacket result;
    {
        std::lock_guard<std::mutex> lock(data.lock);
        for(auto i = data->begin(); i != data->end();) {
            result.emplace_back();
            auto& new_packet  = result.back();
            new_packet.format = (*i).format;

            n_frames in_vector = (*i).get_frames();
            if(in_vector > to_cut) {
                u64 limit = (*i).format.channels * to_cut * (*i).format.get_bytewidth();
                std::copy((*i).pcm.begin(), (*i).pcm.begin() + limit, std::back_inserter(new_packet.pcm));
                (*i).pcm.erase((*i).pcm.begin(), (*i).pcm.begin() + limit);
                new_packet.original_frame_pos[0] = (*i).original_frame_pos[0];
                (*i).original_frame_pos[0] += to_cut;
                new_packet.original_frame_pos[1] = (*i).original_frame_pos[0];
                to_cut = 0;
                i++;
            } else {
                new_packet.pcm = std::move((*i).pcm);
                new_packet.original_frame_pos[0] = (*i).original_frame_pos[0];
                new_packet.original_frame_pos[1] = (*i).original_frame_pos[1];
                i                                = data->erase(i);
                to_cut -= in_vector;
            }
            if(to_cut == 0) break;
        }
    }
    notify_need_fill_buffer();
    return result;
}
PCMFormat Buffer::get_next_format() {
    std::lock_guard<std::mutex> lock(data.lock);
    if(data->empty()){
        PCMFormat result;
        result.sample_type = FORMAT_SAMPLE_TYPE::UNKNOWN;
        return result;
    }
    return data->operator[](0).format;
}
bool Buffer::has_enough_packets(){
    return filled_frame() >= unfreeze_threshold;
}
void Buffer::set_buffer_underrun_handler(std::function<void(void)> handler){
    buffer_underrun_handler = handler;
}
}
