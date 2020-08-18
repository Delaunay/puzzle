#ifndef PUZZLE_APP_LOGGER_HEADER
#define PUZZLE_APP_LOGGER_HEADER

#include <string>
#include <vector>
// Do not include spdlog directly
// only use the fmt header
#include <spdlog/fmt/bundled/core.h>
#include <cassert>

namespace sym
{

struct CodeLocation{
    CodeLocation(std::string const& file, std::string const& fun, int line, std::string const& fun_long):
        filename(file), function_name(fun), line(line), function_long(fun_long)
    {}

    std::string filename;
    std::string function_name;
    int line;
    std::string function_long;
};

#define LOC sym::CodeLocation(__FILE__, __FUNCTION__, __LINE__, __PRETTY_FUNCTION__)

enum class LogLevel{
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL,
    OFF
};

// Show backtrace using spdlog
void show_log_backtrace();

// Show backtrace using execinfo
void show_backtrace();

// retrieve backtrace using execinfo
std::vector<std::string> get_backtrace(size_t size);

void spdlog_log(LogLevel level, const std::string& msg);

template<typename ... Args>
void log(LogLevel level, CodeLocation const& loc, const char* fmt, const Args& ... args){
    auto msg = fmt::format("{}:{} {} - {}",
                           loc.filename,
                           loc.line,
                           loc.function_name,
                           fmt::format(fmt, args...));

    spdlog_log(level, msg);
}

template<typename ... Args>
void new_assert(bool cond, CodeLocation const& loc, const char* pred, const char* fmt, const Args& ... args){
    if (!cond){
        auto msg = fmt::format("{} - {}", pred, fmt::format(fmt, args...));
        // externed
        __assert_fail(msg.c_str(), loc.filename.c_str(), loc.line, loc.function_name.c_str());
    }
}


#define SYM_LOG_HELPER(level, ...) log(level, LOC, __VA_ARGS__)

#define info(...)       SYM_LOG_HELPER(sym::LogLevel::INFO, __VA_ARGS__)
#define warn(...)       SYM_LOG_HELPER(sym::LogLevel::WARN, __VA_ARGS__)
#define debug(...)      SYM_LOG_HELPER(sym::LogLevel::DEBUG, __VA_ARGS__)
#define error(...)      SYM_LOG_HELPER(sym::LogLevel::ERROR, __VA_ARGS__)
#define critical(...)   SYM_LOG_HELPER(sym::LogLevel::CRITICAL, __VA_ARGS__)


// Exception that shows the backtrace when .what() is called
class Exception: public std::exception{
public:
    template<typename ... Args>
    Exception(const char* fmt, const Args& ... args):
        message(fmt::format(fmt, args...).c_str())
    {}

    const char* what() const noexcept final;

private:
    const char* message;
};

// Make a simple exception
#define NEW_EXCEPTION(name)\
    class name: public sym::Exception{\
    public:\
        template<typename ... Args>\
        name(const char* fmt, const Args& ... args):\
            Exception(fmt, args...)\
        {}\
    };
}

// assertf(a > f, "Should happen")
#define assertf(cond, ...)\
    sym::new_assert(cond, LOC, #cond, __VA_ARGS__)

#endif
