#include "sys.h"

#include <vector>
#include <fstream>
#include <stdexcept>

#ifdef __linux__
#include <libgen.h>         // dirname
#include <unistd.h>         // readlink
#include <linux/limits.h>   // PATH_MAX
#else

#endif


std::string _binary_path(){
    const char *path = nullptr;
#ifdef __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);

    if (count != -1) {
        path = dirname(result);
    }
#else
#endif
    return std::string(path);
}


namespace puzzle {
std::string const& binary_path(){
    static std::string loc = _binary_path();
    return loc;
}


std::vector<char> read_file(const std::string& filename, const std::string& folder) {
    std::ifstream file(folder + "/" + filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        RAISE(std::runtime_error, "failed to open file!")
    }

    size_t fileSize = size_t(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), static_cast<std::streamsize>(fileSize));

    file.close();

    return buffer;
}
}
