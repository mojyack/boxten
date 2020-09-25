#include <QSplitter>
#include <config.h>
#include <configuration.hpp>
#include <debug.hpp>
#include <libboxten.hpp>
#include <module.hpp>

#include "config_tool.hpp"
#include "module.hpp"

std::filesystem::path find_config_dir() {
    std::filesystem::path config_dir      = PREFIX;
    auto                  xdg_config_home = std::getenv("XDG_CONFIG_HOME");
    if(xdg_config_home == nullptr) {
        auto home_env = std::getenv("HOME");
        config_dir.append(home_env + 1);
        config_dir.append(".config");
    } else {
        config_dir.append(xdg_config_home + 1);
    }
    config_dir.append("boxten");
    return config_dir;
}
bool get_module_dirs(std::vector<std::filesystem::path>& dirs) {
    constexpr const char* key = "module_path";
    if(std::vector<std::string> str; boxten::config::get_string_array(key, str)) {
        for(auto s : str) {
            dirs.emplace_back(s);
        }
        return true;
    }
    DEBUG_OUT("failed to get module path.");

    std::filesystem::path module_path = PREFIX;
    module_path.append("lib/boxten-modules");
    boxten::config::set_string_array(key, std::vector<std::string>{module_path.string()});
    return true;
}
bool get_input_component(boxten::ComponentName& c_name){
    constexpr const char* key = "input_component";
    std::vector<std::string> str_array;
    if(!boxten::config::get_string_array(key, str_array) || str_array.size() != 2) {
        DEBUG_OUT("failed to get input component path.");
        boxten::config::set_string_array(key, {"Wav input module", "Wav input"});
        return true;
    }else{
        c_name[0] = str_array[0];
        c_name[1] = str_array[1];
        return true;
    }
}
bool get_output_component(boxten::ComponentName& c_name) {
    constexpr const char*    key = "output_component";
    std::vector<std::string> str_array;
    if(!boxten::config::get_string_array(key, str_array) || str_array.size() != 2) {
        DEBUG_OUT("failed to get output component path.");
        boxten::config::set_string_array(key, {"ALSA output module", "ALSA output"});
        return true;
    } else {
        c_name[0] = str_array[0];
        c_name[1] = str_array[1];
        return true;
    }
}
bool apply_layout(BaseWindow& base_window, boxten::LayoutData layout){
    bool result = true;

    std::function<void(const boxten::LayoutData&, QObject&, bool)> install = [&install, &result](const boxten::LayoutData& cfg, QObject& parent, bool is_parent_base_window) {
        if(cfg.type == boxten::LayoutData::WIDGET) {
            auto component = boxten::search_component(cfg.name);
            if(component == nullptr) {
                boxten::console << "cannot find component: " << cfg.name[0] << "/" << cfg.name[1] << std::endl;
                result = false;
                return;
            }
            if(is_parent_base_window) {
                dynamic_cast<QMainWindow*>(&parent)->setCentralWidget(dynamic_cast<QWidget*>(component));
            } else {
                dynamic_cast<QSplitter*>(&parent)->addWidget(dynamic_cast<QWidget*>(component));
            }
        } else {
            QSplitter* layout = new QSplitter(cfg.type == boxten::LayoutData::V_SPLIT ? Qt::Vertical : Qt::Horizontal);
            if(is_parent_base_window) {
                dynamic_cast<QMainWindow*>(&parent)->setCentralWidget(layout);
            } else {
                dynamic_cast<QSplitter*>(&parent)->addWidget(layout);
            }
            for(auto c : cfg.children) {
                install(c, *layout, false);
            }
        }
    };
    install(layout, base_window, true);
    return result;
}