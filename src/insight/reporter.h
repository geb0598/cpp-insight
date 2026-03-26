#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#include "insight/profile_types.h"
#include "insight/scope_profiler.h"

namespace insight {

using SharedFrame = std::shared_ptr<const FrameRecord>;

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
    size_t Size(uint32_t track_id = 0) const;
    size_t TotalSize() const;

    std::vector<GroupSummary>  SummarizeByGroup(size_t count,             uint32_t track_id = 0) const;
    std::vector<StackSummary>  SummarizeByStack(size_t begin, size_t end, uint32_t track_id = 0) const;
    TimelineSummary            GetTimelineSummary(                        uint32_t track_id = 0) const;
    FlameSummary               GetFlameSummary() const;

    std::vector<SharedFrame>   GetTrack(uint32_t track_id) const;
    bool                       HasTrack(uint32_t track_id) const;

private:
    Reporter()  = default;
    ~Reporter() = default;

    TimingSummary ComputeTiming(std::vector<double> ms_values) const;

    std::unordered_map<uint32_t, std::vector<SharedFrame>> tracks_;
    mutable std::mutex                                     mutex_;
};

} // namespace insight
