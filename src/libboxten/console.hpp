#pragma once
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

namespace boxten {
enum class ConsoleType {
    normal,
    warning,
    error,
};

class Console {
  private:
    std::ostringstream         oss;
    std::optional<std::string> prefix;
    const ConsoleType          type;

  public:
    template <class T>
    Console& operator<<(const T& a) {
        oss << a;
        return *this;
    }
    Console& operator=(const Console&)& = delete;
    Console& operator<<(std::ostream& (*pf)(std::ostream&));
    Console(const char* prefix, ConsoleType type);
    Console();
};

struct ConsoleSet {
    Console message;
    Console warning;
    Console error;
    ConsoleSet(const char* prefix);
};
} // namespace boxten
