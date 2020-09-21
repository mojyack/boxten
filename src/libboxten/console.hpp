#pragma once
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace boxten{
class Console{
  private:
    std::ostringstream oss;
  public:
    template <class T>
    Console& operator<<(const T& a) {
        oss << a;
        return *this;
    }
    Console& operator<<(std::ostream& (*pf)(std::ostream&)) {
        oss << pf;

        std::cout << oss.str() << std::endl;
        oss.str("");
        oss.clear();
        return *this;
    }
};
extern Console console;
} // namespace boxten