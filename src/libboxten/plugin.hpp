#pragma once
#include <functional>

#include "audiofile.hpp"
#include "eventhook.hpp"
#include "type.hpp"

namespace boxten {
enum COMPONENT_TYPE {
    SOUND_PROCESSOR,
    STREAM_INPUT,
    STREAM_OUTPUT,
    WIDGET,
};

class Component {
  private:
    std::filesystem::path resource_dir;

  protected:
    std::filesystem::path get_resource_dir();
    void                  install_eventhook(std::function<void(void)> hook, std::initializer_list<EVENT> events);
    
    /* configuration */
    bool get_string(const char* key, std::string& result);
    bool get_string_array(const char* key, std::vector<std::string>& result);
    void set_string_array(const char* key, const std::vector<std::string>& data);

  public:
    const ComponentName                       component_name;
    const COMPONENT_TYPE                      component_type;
    const std::function<void(Component* arg)> free;
    Component(void* param);
    virtual ~Component();
};

class SoundProcessor : public Component {
  protected:
    SoundProcessor* prev;
    SoundProcessor* next;

  public:
    SoundProcessor(void* param) : Component(param) {}
    virtual ~SoundProcessor(){}
};

class StreamInput : public Component {
  public:
    virtual PCMPacketUnit read_frames(AudioFile& file, u64 from, n_frames frames) = 0;
    virtual n_frames      calc_total_frames(AudioFile& file)                      = 0;
    virtual AudioTag      read_tags(AudioFile& file)                              = 0;
    StreamInput(void* param) : Component(param) {}
    virtual ~StreamInput(){}
};

class StreamOutput : public Component {
  protected:
    static n_frames  get_buffer_filled_frames();
    static PCMPacket get_buffer_pcm_packet(n_frames frames);
    static PCMFormat get_buffer_pcm_format();

  public:
    virtual n_frames output_delay(); // delay between get_buffer_pcm_packet() and speaker sounds.
    virtual void     start_playback()  = 0;
    virtual void     stop_playback()   = 0;
    virtual void     pause_playback()  = 0;
    virtual void     resume_playback() = 0;
    StreamOutput(void* param) : Component(param) {}
    virtual ~StreamOutput(){}
};

class Widget : public Component {
  public:
    Widget(void* param) : Component(param) {}
    virtual ~Widget(){}
};

struct ComponentInfo {
    std::string    name;
    COMPONENT_TYPE type;
    std::function<Component*(void* arg)> alloc;
    std::function<void(Component* arg)>  free;
    ComponentInfo(std::string name, COMPONENT_TYPE type,
                  std::function<Component*(void* arg)> alloc,
                  std::function<void(Component* arg)>  free) : name(name), type(type), alloc(alloc), free(free) {}
};

#define CATALOGUE_CALLBACK(component_name)         \
    [](void* arg) -> boxten::Component* {          \
        return new component_name(arg);            \
    },                                             \
    [](boxten::Component* arg) {                   \
        delete dynamic_cast<component_name*>(arg); \
    }

class Module {
  public:
    const char*                     module_name;
    std::vector<ComponentInfo>      component_catalogue;
    virtual ~Module() = 0;
};
} // namespace boxten