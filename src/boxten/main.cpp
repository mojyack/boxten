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

#include "gui.hpp"
#include "config_tool.hpp"

int main(int argc, char* argv[]) {
    boxten::console << "boxten" << std::endl;

    /* start threads */
    boxten::start_master_thread();
    boxten::start_playback_thread();
    boxten::start_hook_invoker();

    auto config_dir = find_config_dir();
    if(!boxten::config::set_config_dir(config_dir)) {
        boxten::console << "failed to set configuration directory: " << config_dir << std::endl;
        exit(1);
    }

    if(std::vector<std::filesystem::path> module_dirs; !get_module_dirs(module_dirs)) {
        exit(1);
    } else {
        boxten::open_modules(module_dirs);
    }

    /* set input&output component */
    {
        boxten::ComponentName input_component_name;
        get_input_component(input_component_name);
        auto input_component = boxten::search_component(input_component_name);
        if(input_component == nullptr) {
            boxten::console << "cannot find input conponent" << std::endl;
            exit(1);
        }
        boxten::set_stream_input(dynamic_cast<boxten::StreamInput*>(input_component));
    }
    {
        boxten::ComponentName output_component_name;
        get_output_component(output_component_name);
        auto output_component  = boxten::search_component(output_component_name);
        if(output_component == nullptr) {
            boxten::console << "cannot find output conponent" << std::endl;
            exit(1);
        }
        boxten::set_stream_output(dynamic_cast<boxten::StreamOutput*>(output_component));
    }

    /* load layout */
    QApplication application(argc, argv);
    BaseWindow   base_window;
    boxten::LayoutData layout;
    if(!boxten::config::get_layout_config(layout)) {
        DEBUG_OUT("cannot load layout config.");
        exit(1);
    }
    if(!apply_layout(base_window, layout)) {
        boxten::console << "cannot set layout." << std::endl;
        exit(1);
    }

    base_window.show();
    application.exec();
    boxten::stop_playback();

    boxten::finish_hook_invoker();
    boxten::finish_playback_thread();
    boxten::finish_master_thread().join();
    boxten::console << std::endl;
    return 0;
}