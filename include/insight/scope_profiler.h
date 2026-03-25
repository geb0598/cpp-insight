#pragma once

#include "insight/archive.h"
#include "insight/platform_time.h"
#include "insight/registry.h"

namespace insight {

// -------------------------------------------------
// ScopeRecord
// -------------------------------------------------
struct ScopeRecord {
    Descriptor::Id  id;
    int64_t         start_ns;
    int64_t         end_ns;
    int             depth;
    uint32_t        track_id;
};

inline Archive& operator<<(Archive& ar, ScopeRecord& record) {
    ar << record.id;
    ar << record.start_ns;
    ar << record.end_ns;
    ar << record.depth;
    ar << record.track_id;
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

    void BeginRecording() {
        recording_start_      = PlatformTime::Now();
        is_recording_started_ = true;
    }

    void EndRecording() {
        is_recording_started_ = false;
        is_frame_started_     = false;
        stack_.clear();
    }

    void BeginFrame() {
        frame_.clear();
        if (is_recording_started_) {
            is_frame_started_ = true;
            BeginScope(Descriptor::GetFrameDescriptor());
        }
    }

    FrameRecord EndFrame() {
        if (is_frame_started_) {
            EndScope();
            is_frame_started_ = false;
        }
        return std::move(frame_);
    }

    void BeginScope(const Descriptor& stat) {
        if (is_frame_started_) {
            int depth = static_cast<int>(stack_.size());
            stack_.push_back({stat.GetId(), PlatformTime::Now(), depth});
        }
    }

    void EndScope() {
        if (is_frame_started_) {
            auto& entry      = stack_.back();
            auto    now      = PlatformTime::Now();
            int64_t start_ns = PlatformTime::Elapsed(recording_start_, entry.start).count();
            int64_t end_ns   = PlatformTime::Elapsed(recording_start_, now).count();
            frame_.push_back({ entry.id, start_ns, end_ns, entry.depth, 0u });
            stack_.pop_back();
        }
    }

    void Clear() {
        stack_.clear();
        frame_.clear();
        is_frame_started_ = false;
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

    bool                    is_recording_started_ = false;
    bool                    is_frame_started_     = false;
    PlatformTime::TimePoint recording_start_;
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