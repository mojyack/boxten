#include <fstream>
#include <iostream>

#include "configuration.hpp"
#include "console.hpp"
#include <config.h>


namespace boxten::config {
std::filesystem::path config_home_dir;
std::filesystem::path config_path;
nlohmann::json        config_data;
std::mutex            config_data_lock;

bool set_config_dir(std::filesystem::path config_dir){
    if(!std::filesystem::is_directory(config_dir)) return false;
    config_home_dir = config_dir;
    {
        config_path = config_home_dir;
        config_path.append("config.json");
        if(!std::filesystem::is_regular_file(config_path)) return true;
        std::ifstream config_file(config_path);
        config_file >> config_data;
    }
    return true;
}
nlohmann::json& get_configuration_ref(){
    return config_data;
}
std::mutex&     get_configuration_lock_ref(){
    return config_data_lock;
}
void save_config() {
    std::lock_guard<std::mutex> lock(config_data_lock);
    std::ofstream config_file(config_path);
    config_file << std::setw(4) << config_data << std::endl;
}
} // namespace boxten