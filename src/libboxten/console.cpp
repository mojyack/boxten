#include "console.hpp"

namespace {
constexpr const char* colors[3] = {
    "\x1b[37m",
    "\x1b[33m",
    "\x1b[31m",
};
}
namespace boxten {
Console& Console::operator<<(std::ostream& (*pf)(std::ostream&)) {
    oss << pf;

    std::string text;
    if(prefix.has_value()) text = prefix.value();
    text += oss.str();
    if(type != ConsoleType::normal) {
        std::cout << colors[type == ConsoleType::warning ? 1 : 2];
    }
    std::cout << text << std::endl;
    if(type != ConsoleType::normal) {
        std::cout << colors[0];
    }

    oss.str("");
    oss.clear();
    return *this;
}
Console::Console(const char* prefix, ConsoleType type) : prefix(prefix), type(type) {}
Console::Console() : type(ConsoleType::normal) {}

ConsoleSet::ConsoleSet(const char* prefix) : message(Console(prefix, ConsoleType::normal)),
                                             warning(Console(prefix, ConsoleType::warning)),
                                             error(Console(prefix, ConsoleType::error)) {}
} // namespace boxten
