#pragma once

#include <vector>

#include "insight/profile_types.h"
#include "insight/scope_profiler.h"

namespace insight {

// -------------------------------------------------
// Reporter
// -------------------------------------------------
class Reporter {
public:
    static Reporter& GetInstance() {
        static Reporter instance;
        return instance;
    }

    Reporter(const Reporter&)            = delete;
    Reporter& operator=(const Reporter&) = delete;
    Reporter(Reporter&&)                 = delete;
    Reporter& operator=(Reporter&&)      = delete;

    void   Submit(FrameRecord frame);
    void   Clear();
    size_t Size() const { return frames_.size(); }

    const std::vector<FrameRecord>& GetFrames() const { return frames_; }

    std::vector<GroupSummary> SummarizeByGroup(size_t count) const;
    std::vector<StackSummary> SummarizeByStack(size_t begin, size_t end) const;

private:
    Reporter()  = default;
    ~Reporter() = default;

    TimingSummary ComputeTiming(std::vector<double> ms_values) const;

    std::vector<FrameRecord> frames_;
};

} // namespace insight