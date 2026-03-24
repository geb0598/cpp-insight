#pragma once

#include <optional>
#include <vector>

#include "insight/registry.h"

namespace insight {

struct TimingSummary {
    double                avg_ms;
    double                min_ms;
    double                max_ms;
    std::optional<double> p95_ms;
    std::optional<double> p99_ms;
};

struct StatSummary {
    Descriptor::Id id;
    TimingSummary  timing;
};

struct GroupSummary {
    Group::Id                group_id;
    std::vector<StatSummary> stats;
};

struct StackSummary {
    Descriptor::Id id;
    Descriptor::Id parent_id;
    int            depth;
    TimingSummary  inclusive;
    TimingSummary  exclusive;
    size_t         count;
};

struct TimelineSummary {
    std::vector<float>                                     total_frame_ms;
    std::vector<float>                                     unaccounted_ms;
    std::unordered_map<Descriptor::Id, std::vector<float>> tracks;
};

} // namespace insight