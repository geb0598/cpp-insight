#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <deque>

#include "insight/scope_profiler.h"

namespace insight {

class StatAggregator {
public:
    static constexpr size_t MaxRealtimeFrames = 300;
    static constexpr size_t MaxSessionFrames = 18000;

    static StatAggregator& GetInstance() {
        static StatAggregator instance;
        return instance;
    }

    StatAggregator(const StatAggregator&)           = delete;
    StatAggregator operator=(const StatAggregator&) = delete;
    StatAggregator(StatAggregator&&)                = delete;
    StatAggregator operator=(StatAggregator&&)      = delete;

    void Push(FrameRecord frame) {
        frames_.push_back(std::move(frame));

        while (!is_session_active_ && frames_.size() > MaxRealtimeFrames) {
            frames_.pop_front();
        }

        if (is_session_active_ && frames_.size() - session_start_index_ > MaxSessionFrames) {
            StopSession();
        }
    }

    const std::deque<FrameRecord>& GetFrames() const { return frames_; }

    void StartSession() {
        session_start_index_ = frames_.size();
        is_session_active_ = true;
    }

    void StopSession() { is_session_active_ = false; }

    bool IsSessionActive() const { return is_session_active_; }

private:
    StatAggregator()  = default;
    ~StatAggregator() = default;

    std::deque<FrameRecord> frames_;
    bool                    is_session_active_ = false;
    size_t                  session_start_index_ = 0;
};

} // namespace insight