#ifndef PUZZLE_APP_UTILS_HEADER
#define PUZZLE_APP_UTILS_HEADER

#include <chrono>
#include <ratio>
#include <cmath>
#include <atomic>

#include "logger.h"

class TimeIt {
  public:
    using TimePoint       = std::chrono::high_resolution_clock::time_point;
    using Clock           = std::chrono::high_resolution_clock;
    TimePoint const start = Clock::now();

    double stop() const {
        TimePoint end = Clock::now();
        std::chrono::duration<double> time_span =
            std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
        return time_span.count();
    }

    TimeIt operator=(TimeIt p){
        return TimeIt(p);
    }

    TimeIt(const TimeIt& p):
        start(p.start)
    {}

    TimeIt() = default;
};

template<typename T>
void add(std::atomic<T>& acc, T val){
    acc.store(acc.load() + val);
}

struct RuntimeStatistics {
    static RuntimeStatistics& stat(){
        static RuntimeStatistics st;
        return st;
    }

};

struct StatStream {
    StatStream &operator+=(double val) {
        count += 1;
        sum += val;
        sum2 += val * val;
        return *this;
    }

    bool less_than(StatStream const &stats) const { return mean() < stats.mean(); }
    bool greater_than(StatStream const &stats) const { return mean() < stats.mean(); }

    bool equal(StatStream const &) const { return false; }
    bool different(StatStream const &) const { return true; }

    double mean() const { return sum / double(count); }
    double var()  const { return sum2 / double(count) - mean() * mean(); }
    double sd()   const { return sqrt(var()); }

    double sum        = 0;
    double sum2       = 0;
    std::size_t count = 0;
};

#endif

