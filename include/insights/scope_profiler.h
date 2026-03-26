#pragma once

#include <functional>

#include "insights/archive.h"
#include "insights/platform_time.h"
#include "insights/registry.h"

namespace insights {

// -------------------------------------------------
// TrackId
// -------------------------------------------------
namespace TrackId {
    constexpr uint32_t CPU_BASE = 0x00000000u;  // 0, 1, 2, ... (one per thread)
    constexpr uint32_t GPU_BASE = 0x80000000u;  // 0x80000000, 0x80000001, ... (one per GPU device)
}

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

    void SetOnBeginRecording(std::function<void()> fn) {
        on_begin_recording_ = std::move(fn);
    }

    void SetOnEndRecording(std::function<void()> fn) {
        on_end_recording_ = std::move(fn);
    }

    void BeginRecording() {
        first_frame_pending_  = true;
        is_recording_started_ = true;
        if (on_begin_recording_) {
            on_begin_recording_();
        }
    }

    void EndRecording() {
        first_frame_pending_  = false;
        is_recording_started_ = false;
        is_frame_started_     = false;
        stack_.clear();
        if (on_end_recording_) {
            on_end_recording_();
        }
    }

    PlatformTime::TimePoint GetRecordingStart() const {
        return recording_start_;
    }

    void BeginFrame() {
        frame_.clear();
        if (is_recording_started_) {
            if (first_frame_pending_) {
                recording_start_     = PlatformTime::Now();
                first_frame_pending_ = false;
            }
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

    bool                    first_frame_pending_  = false;
    bool                    is_recording_started_ = false;
    bool                    is_frame_started_     = false;
    PlatformTime::TimePoint recording_start_;

    std::function<void()>   on_begin_recording_;
    std::function<void()>   on_end_recording_;
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

} // namespace insights