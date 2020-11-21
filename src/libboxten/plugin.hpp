#pragma once
#include <functional>

#include "audiofile.hpp"
#include "eventhook.hpp"
#include "json.hpp"
#include "type.hpp"

namespace boxten {
enum COMPONENT_TYPE {
    SOUND_PROCESSOR,
    STREAM_INPUT,
    STREAM_OUTPUT,
    WIDGET,
};

class Configurator {
  private:
    std::string domain;

  protected:
    bool get_string(const char* key, std::string& result);
    bool get_string_array(const char* key, std::vector<std::string>& result);
    void set_string_array(const char* key, const std::vector<std::string>& data);
    bool get_number(const char* key, i64& result);
    bool load_configuration(nlohmann::json& config_data);
    bool save_configuration(const nlohmann::json& config_data);

    Configurator(const char* domain);
};
class Component : public Configurator {
  private:
    std::filesystem::path     resource_dir;
    std::function<void(void)> decrement_component_count;

  protected:
    std::filesystem::path get_resource_dir();
    void                  install_eventhook(std::function<void(void)> hook, std::initializer_list<EVENT> events);

  public:
    const ComponentName  component_name;
    const COMPONENT_TYPE component_type;
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
typedef std::vector<ComponentInfo> ComponentCatalogue;

#define BOXTEN_MODULE(...)                                                           \
    extern "C" const char                       module_name[]       = MODULE_NAME;   \
    extern "C" const boxten::ComponentCatalogue component_catalogue = {__VA_ARGS__}; \

#define BOXTEN_MODULE_CLASS(ON_LOAD_FUNC, ON_UNLOAD_FUNC)              \
    class BoxtenModule : public boxten::Module {                       \
      public:                                                          \
        BoxtenModule() : boxten::Module(MODULE_NAME) { ON_LOAD_FUNC; } \
        ~BoxtenModule() { ON_UNLOAD_FUNC; }                            \
    };                                                                 \
    extern "C" {                                                       \
    boxten::Module* install_module() {                                 \
        return new BoxtenModule;                                       \
    }                                                                  \
    void uninstall_module(boxten::Module* module) {                    \
        delete module;                                                 \
    }                                                                  \
    }

#define CATALOGUE_CALLBACK(component_name)         \
    [](void* arg) -> boxten::Component* {          \
        return new component_name(arg);            \
    },                                             \
    [](boxten::Component* arg) {                   \
        delete dynamic_cast<component_name*>(arg); \
    }

class Module : public Configurator {
  public:
    const char*        module_name;
    ComponentCatalogue component_catalogue;
    Module(const char* module_name);
    virtual ~Module();
};
} // namespace boxten