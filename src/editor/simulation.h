#ifndef PUZZLE_SIMULATION_HEADER
#define PUZZLE_SIMULATION_HEADER

#include "config.h"
#include <unordered_map>
#include <spdlog/fmt/bundled/format.h>

struct DoubleEntry{
    float debit  = 0.f;
    float credit = 0.f;

    DoubleEntry& operator+=(float v){
        debit += v;
        return *this;
    }

    DoubleEntry& operator-=(float v){
        credit += v;
        return *this;
    }

    float operator/ (float d) const {
        return total() / d;
    }

    float total() const {
        return debit - credit;
    }
};

inline
std::ostream& operator<<(std::ostream& out, DoubleEntry const& ent){
    return out << ent.total();
}

template <>
struct fmt::formatter<DoubleEntry>: formatter<float> {
    template <typename FormatContext>
    auto format(DoubleEntry c, FormatContext& ctx) {
        return formatter<float>::format(c.total(), ctx);
    }
};

struct ItemStat{
    float consumed;
    float produced;
    float received;

    float overflow   = 0;
    float efficiency = 0;

    float limit_consumed = 0;
    float limit_produced = 0;
    float limit_received = 0;
};

struct Engery{
    float consumed = 0;
    float produced = 0;
    std::unordered_map<Building*, float> stats;
};

using ProductionBook  = std::unordered_map<std::string, ItemStat>;
using ProductionStats = std::unordered_map<std::size_t, ProductionBook>;
using CircleGuard     = std::unordered_map<std::size_t, bool>;


struct Simulator{

};
#endif
