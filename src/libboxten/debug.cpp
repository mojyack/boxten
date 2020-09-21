#include <vector>

#include "debug.hpp"

#ifdef DEBUG
namespace{
constexpr const char* default_color = "\x1b[49m";
constexpr const char* colors[]      = {
    "\x1b[41m",
    "\x1b[42m",
    "\x1b[43m",
    "\x1b[44m",
    "\x1b[45m",
    "\x1b[46m",
    "\x1b[47m",
};
constexpr unsigned int           colors_size = sizeof(colors) / sizeof(colors[0]);
static std::vector<unsigned int> assigned_colors;
static std::mutex                mutex;
int get_color(unsigned int thread_id){
    std::lock_guard<std::mutex> lock(mutex);
    int                         color = -1;
    for(unsigned int i = 0; i < assigned_colors.size(); ++i) {
        if(assigned_colors[i] == thread_id) {
            color = i % colors_size;
            break;
        }
    }
    if(color == -1) {
        assigned_colors.emplace_back(thread_id);
        color = (assigned_colors.size() % colors_size) - 1;
    }
    return color;
}
}
void debug_pring_worker_start(const char* file, int line, char icon){
    auto thread_id = static_cast<unsigned int>(pthread_self());
    auto color     = get_color(thread_id);
    printf(colors[color]);
    printf("%c Worker (%s, %d) %08X", icon, file, line, thread_id);
    printf("%s\n", default_color);
} 
void debug_print_mutex_info(const char* name, const char* func, const char* file, int line, char icon) {
    auto                        thread_id = static_cast<unsigned int>(pthread_self());
    auto                        color     = get_color(thread_id);
    printf(colors[color]);
    printf("%c %s at %s() (%s, %d) %08X", icon, name, func, file, line, thread_id);
    printf("%s\n", default_color);
}
#endif