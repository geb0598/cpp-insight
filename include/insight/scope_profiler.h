#pragma once

#include "insight/archive.h"
#include "insight/platform_time.h"
#include "insight/registry.h"

namespace insight {

// -------------------------------------------------
// ScopeRecord
// -------------------------------------------------
struct ScopeRecord {
    Descriptor::Id         id;
    PlatformTime::Duration duration;
    int                    depth;
};

inline Archive& operator<<(Archive& ar, ScopeRecord& record) {
    ar << record.id;
    ar << record.duration;
    ar << record.depth;
    return ar;
}

// -------------------------------------------------
// ScopeProfiler
// -------------------------------------------------
using FrameRecord = std::vector<ScopeRecord>;

class ScopeProfiler {
public:
    static ScopeProfiler& GetInstance() {
        static ScopeProfiler instance;
        return instance;
    }

    ScopeProfiler(const ScopeProfiler&)            = delete;
    ScopeProfiler& operator=(const ScopeProfiler&) = delete;
    ScopeProfiler(ScopeProfiler&&)                 = delete;
    ScopeProfiler& operator=(ScopeProfiler&&)      = delete;

    void BeginFrame() {
        frame_.clear();
        BeginScope(Descriptor::GetFrameDescriptor());
    }

    FrameRecord EndFrame() {
        EndScope();
        return std::move(frame_);
    }

    void BeginScope(const Descriptor& stat) {
        int depth = static_cast<int>(stack_.size());
        stack_.push_back({stat.GetId(), PlatformTime::Now(), depth});
    }

    void EndScope() {
        auto& entry = stack_.back();
        PlatformTime::Duration duration = PlatformTime::Elapsed(entry.start, PlatformTime::Now());
        frame_.push_back({ entry.id, duration, entry.depth });
        stack_.pop_back();
    }

    void Clear() {
        stack_.clear();
        frame_.clear();
    }

private:
    struct StackEntry {
        Descriptor::Id          id;
        PlatformTime::TimePoint start;
        int                     depth;
    };

    ScopeProfiler()  = default;
    ~ScopeProfiler() = default;

    std::vector<StackEntry> stack_;
    FrameRecord             frame_;
};

// -------------------------------------------------
// ScopeTimer 
// -------------------------------------------------
class ScopeTimer {
public:
    explicit ScopeTimer(const Descriptor& stat) {
        ScopeProfiler::GetInstance().BeginScope(stat);
    }

    ~ScopeTimer() {
        ScopeProfiler::GetInstance().EndScope();
    }

    ScopeTimer(const ScopeTimer&)            = delete;
    ScopeTimer& operator=(const ScopeTimer&) = delete;
    ScopeTimer(ScopeTimer&&)                 = delete;
    ScopeTimer& operator=(ScopeTimer&&)      = delete;
};

} // namespace insight