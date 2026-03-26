#pragma once

#include <chrono>

#include "insights/archive.h"

namespace insights {

class PlatformTime {
public:
    using Clock     = std::chrono::steady_clock;
    using Duration  = std::chrono::nanoseconds;
    using TimePoint = Clock::time_point;

    static TimePoint Now() noexcept {
        return Clock::now();
    }

    static Duration Elapsed(TimePoint start, TimePoint end) noexcept {
        return std::chrono::duration_cast<Duration>(end - start);
    }

    template <typename Period>
    static double To(Duration duration) noexcept {
        return std::chrono::duration<double, Period>(duration).count();
    }

    static double ToMicro(Duration duration) noexcept {
        return To<std::micro>(duration);
    }

    static double ToMilli(Duration duration) noexcept {
        return To<std::milli>(duration);
    }

    static double ToSeconds(Duration duration) noexcept {
        return To<std::ratio<1>>(duration);
    }
};

inline Archive& operator<<(Archive& ar, PlatformTime::Duration& duration) {
    int64_t count = duration.count();
    ar << count;
    if (ar.IsLoading()) { 
        duration = PlatformTime::Duration(count); 
    }
    return ar;
}

} // namespace insights