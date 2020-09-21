#include <QSplitter>
#include <config.h>
#include <configuration.hpp>
#include <libboxten.hpp>
#include <module.hpp>

#include "config_tool.hpp"
#include "module.hpp"


namespace{
enum JSON_TYPE {
    OBJECT,
    STRING,
    ARRAY,
};
bool check_json_contents(const char* key, const nlohmann::json& cfg) {
    if(!cfg.contains(key)) {
        boxten::console << "key " << key << " missing" << std::endl;
        return false;
    }
    return true;
}
bool check_json_type(const char* key, JSON_TYPE type, const nlohmann::json& cfg){
    if(!check_json_contents(key, cfg)) return false;
    bool result;
    switch(type) {
    case JSON_TYPE::OBJECT:
        result = cfg[key].is_object();
        break;
    case JSON_TYPE::STRING:
        result = cfg[key].is_string();
        break;
    case JSON_TYPE::ARRAY:
        result = cfg[key].is_array();
        break;
    }
    if(!result){
        boxten::console << "key " << key << "type mismatch " << std::endl;
    }
    return result;
}
}

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

std::vector<std::filesystem::path> get_module_dirs() {
    auto& config_data = boxten::config::get_configuration_ref();
    std::lock_guard<std::mutex> lock(boxten::config::get_configuration_lock_ref());
#define TEST(cond) \
    if(!(cond)) break
    bool failback = true;
    do {
        TEST(!config_data.empty());
        TEST(config_data.contains("module_path"));
        auto entry = config_data["module_path"];
        TEST(entry.is_array());
        TEST([&]() -> bool {
            for(auto& i : entry) {
                if(!i.is_string()) return false;
            }
            return true;
        }());
        failback = false;
    } while(0);
#undef TEST
    if(failback) {
        boxten::console << "configuration \"module_path\" is invalid." << std::endl;
        std::filesystem::path module_path = PREFIX;
        module_path.append("lib/boxten-modules");
        config_data["module_path"] = std::vector<std::filesystem::path>{module_path};
        boxten::config::save_config();
    }
    return config_data["module_path"].get<std::vector<std::filesystem::path>>();
}
std::array<std::string, 2> get_input_component() {
    auto&                       config_data = boxten::config::get_configuration_ref();
    std::lock_guard<std::mutex> lock(boxten::config::get_configuration_lock_ref());
#define TEST(cond) \
    if(!(cond)) break
    bool failback = true;
    do {
        TEST(!config_data.empty());
        TEST(config_data.contains("input_component"));
        auto entry = config_data["input_component"];
        TEST(entry.is_array());
        TEST(entry.size() == 2);
        TEST(entry[0].is_string() && entry[1].is_string());
        failback = false;
    } while(0);
#undef TEST
    if(failback) {
        boxten::console << "configuration \"input_component\" is invalid." << std::endl;
        config_data["input_component"] = std::array<std::string, 2>{"Wav input module", "Wav input"};
        boxten::config::save_config();
    }
    return config_data["input_component"].get<std::array<std::string, 2>>();
}
std::array<std::string, 2> get_output_component() {
    auto&                       config_data = boxten::config::get_configuration_ref();
    std::lock_guard<std::mutex> lock(boxten::config::get_configuration_lock_ref());
#define TEST(cond) \
    if(!(cond)) break
    bool failback = true;
    do {
        TEST(!config_data.empty());
        TEST(config_data.contains("output_component"));
        auto entry = config_data["output_component"];
        TEST(entry.is_array());
        TEST(entry.size() == 2);
        TEST(entry[0].is_string() && entry[1].is_string());
        failback = false;
    } while(0);
#undef TEST
    if(failback) {
        boxten::console << "configuration \"output_component\" is invalid." << std::endl;
        config_data["output_component"] = std::array<std::string, 2>{"ALSA output module", "ALSA output"};
        boxten::config::save_config();
    }
    return config_data["output_component"].get<std::array<std::string, 2>>();
}
bool load_layout(BaseWindow& base_window) {
    auto&                       config_data = boxten::config::get_configuration_ref();
    std::lock_guard<std::mutex> lock(boxten::config::get_configuration_lock_ref());
    nlohmann::json              layout_config;
#define TEST(cond) \
    if(!(cond)) break
    {
        bool failback = true;
        do {
            TEST(!config_data.empty());
            TEST(config_data.contains("layout"));
            layout_config = config_data["layout"];
            TEST(layout_config.is_object());
            failback = false;
        } while(0);
        if(failback){
            boxten::console << "configuration \"layout\" is invalid." << std::endl;
            return false;
        }
#undef TEST
    }
    bool result = true;
    std::function<void(const nlohmann::json&, QObject&, bool)> parse_layout = [&](const nlohmann::json& cfg, QObject& parent, bool is_parent_base_window) {
        if(!check_json_type("type", JSON_TYPE::STRING, cfg)){
            result = false;
            return;
        }
        auto type = cfg["type"].get<std::string>();
        bool vlayout = type == "vlayout";
        bool hlayout = !vlayout && type == "hlayout";
        bool widget  = !vlayout && !hlayout && type == "widget";
        if(!vlayout && !hlayout && !widget) {
            boxten::console << "key \"type\" is invalid" << std::endl;
            result = false;
            return;
        }
        if(widget){
            if(!check_json_type("name", JSON_TYPE::ARRAY, cfg)) {
                result = false;
                return;
            }
            auto name_cfg = cfg["name"];
            if(name_cfg.size() != 2 || !name_cfg[0].is_string() || !name_cfg[1].is_string()) {
                boxten::console << "widget name invalid" << std::endl;
                result = false;
                return;
            }
            auto name      = name_cfg.get<std::array<std::string, 2>>();
            auto component = boxten::search_component(name);
            if(component == nullptr) {
                boxten::console << "cannot find component: " << name[0] << "/" << name[1] << std::endl;
                result = false;
                return;
            }
            if(is_parent_base_window) {
                dynamic_cast<QMainWindow*>(&parent)->setCentralWidget(dynamic_cast<QWidget*>(component));
            } else {
                dynamic_cast<QSplitter*>(&parent)->addWidget(dynamic_cast<QWidget*>(component));
            }
        }else{
            if(!check_json_type("children", JSON_TYPE::ARRAY, cfg)) {
                result = false;
                return;
            }
            QSplitter* layout = new QSplitter(vlayout ? Qt::Vertical : Qt::Horizontal);
            if(is_parent_base_window) {
                dynamic_cast<QMainWindow*>(&parent)->setCentralWidget(layout);
            } else {
                dynamic_cast<QSplitter*>(&parent)->addWidget(layout);
            }
            for(auto c : cfg["children"]) {
                parse_layout(c, *layout, false);
            }
        }
    };
    parse_layout(layout_config, base_window, true);
    return result;
}