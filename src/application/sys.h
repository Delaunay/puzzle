#ifndef PUZZLE_APP_SYS_HEADER
#define PUZZLE_APP_SYS_HEADER

#include <string>
#include "logger.h"

#define RAISE(exception, message)\
{\
    debug("{}: {}", #exception, message);\
    throw exception(message);\
}

namespace puzzle {
std::string const& binary_path();

std::vector<char> read_file(const std::string& filename, const std::string& folder);
}

#endif
