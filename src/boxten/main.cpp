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

    boxten::open_modules(get_module_dirs());

    /* set input&output component */
    {
        auto input_component = boxten::search_component(get_input_component());
        if(input_component == nullptr) {
            boxten::console << "cannot find input conponent" << std::endl;
            exit(1);
        }
        boxten::set_stream_input(dynamic_cast<boxten::StreamInput*>(input_component));
    }
    {
        auto output_component = boxten::search_component(get_output_component());
        if(output_component == nullptr) {
            boxten::console << "cannot find output conponent" << std::endl;
            exit(1);
        }
        boxten::set_stream_output(dynamic_cast<boxten::StreamOutput*>(output_component));
    }

    /* load layout */
    QApplication application(argc, argv);
    BaseWindow   base_window;
    if(!load_layout(base_window)) {
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