#include <functional>

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <config.h>
#include <configuration.hpp>
#include <eventhook_internal.hpp>
#include <libboxten.hpp>
#include <module.hpp>
#include <playback_internal.hpp>
#include <worker_internal.hpp>

#include "config_tool.hpp"
#include "console.hpp"
#include "global.hpp"
#include "gui.hpp"
#include "plugin.hpp"
#include "type.hpp"

boxten::ConsoleSet console("[boxten] ");

int main(int argc, char* argv[]) {
    console.message << "boxten" << std::endl;

    /* start threads */
    boxten::start_master_thread();
    boxten::start_playback_thread();
    boxten::start_hook_invoker();

    auto config_dir = find_config_dir();
    if(!boxten::config::set_config_dir(config_dir)) {
        console.error << "failed to set configuration directory: " << config_dir << std::endl;

        exit(1);
    }

    /* load modules */
    if(std::vector<std::filesystem::path> module_dirs; !get_module_dirs(module_dirs)) {
        exit(1);
    } else {
        boxten::scan_modules(module_dirs);
    }
    boxten::Component *input_component, *output_component;
    std::vector<boxten::SoundProcessor*> sound_processors;
    /* set input&output component */
    {
        boxten::ComponentName input_component_name;
        get_input_component(input_component_name);
        input_component = boxten::search_component(input_component_name);
        if(input_component == nullptr) {
            console.error << "cannot find input conponent.";
            exit(1);
        }
        boxten::set_stream_input(dynamic_cast<boxten::StreamInput*>(input_component));
    }
    {
        boxten::ComponentName output_component_name;
        get_output_component(output_component_name);
        output_component = boxten::search_component(output_component_name);
        if(output_component == nullptr) {
            console.error << "cannot find output conponent.";
            exit(1);
        }
        boxten::set_stream_output(dynamic_cast<boxten::StreamOutput*>(output_component));
    }
    /* load dsp */
    {
        std::vector<boxten::ComponentName> dsp_names;
        get_dsp_chain_component(dsp_names);
        for(auto& n:dsp_names){
            auto c = dynamic_cast<boxten::SoundProcessor*>(boxten::search_component(n));
            if(c == nullptr){
                console.error << "cannot find dsp component.";
            }else{
                sound_processors.emplace_back(c); 
            }
        }
        boxten::set_dsp_chain(sound_processors);
    }

    /* load layout */
    QApplication       application(argc, argv);
    auto               base_window = new BaseWindow;
    boxten::LayoutData layout;
    if(!boxten::config::get_layout_config(layout)) {
        DEBUG_OUT("cannot load layout config.");
        exit(1);
    }
    if(!apply_layout(*base_window, layout)) {
        console.error << "cannot set layout." << std::endl;
        exit(1);
    }

    base_window->show();
    application.exec();
    boxten::stop_playback(true);
    delete base_window;

    boxten::close_component(input_component);
    boxten::close_component(output_component);
    for(auto c:sound_processors){
        boxten::close_component(c);
    }
    boxten::free_modules();

    boxten::finish_hook_invoker();
    boxten::finish_playback_thread();
    boxten::finish_master_thread().join();
    return 0;
}
