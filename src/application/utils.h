#ifndef PUZZLE_APP_UTILS_HEADER
#define PUZZLE_APP_UTILS_HEADER

#include <chrono>
#include <ratio>
#include <cmath>
#include <atomic>
#include <algorithm>

#include "logger.h"

class TimeIt {
  public:
    using TimePoint       = std::chrono::high_resolution_clock::time_point;
    using Duration        = std::chrono::duration<double, std::milli>;
    using Clock           = std::chrono::high_resolution_clock;
    TimePoint const start = Clock::now();

    double stop() const {
        TimePoint end = Clock::now();
        Duration time_span = std::chrono::duration_cast<Duration>(end - start);
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


template<std::size_t N>
struct RollingAverage {
    float size() const {
        return std::max(_size, N);
    }

    float sum() const {
        float s = 0;
        auto n = size();
        for (int i = 0; i < n; ++i){
            s += values[i];
        }
        return s;
    }

    float mean() const {
        return sum() / size();
    }

    float fps() const {
        return 1000.0 / mean();
    }

    template<typename T>
    RollingAverage& operator+= (T val) {
        values[_size] = val;
        _size = (_size + 1) % N;
        return *this;
    }

    std::size_t _size = 0;
    std::array<float, N> values;
};

#endif

