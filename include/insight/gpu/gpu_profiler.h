#pragma once

#include <atomic>
#include <memory>

#include "insight/gpu/gpu_profiler_backend.h"
#include "insight/scope_profiler.h"

namespace insight {

// -------------------------------------------------
// GpuProfiler
// -------------------------------------------------
class GpuProfiler {
public:
    static GpuProfiler& GetInstance() {
        static GpuProfiler instance;
        return instance;
    }

    GpuProfiler(const GpuProfiler&)            = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;
    GpuProfiler(GpuProfiler&&)                 = delete;
    GpuProfiler& operator=(GpuProfiler&&)      = delete;

    void Init(std::unique_ptr<IGpuProfilerBackend> backend);

    void BeginRecording();
    void EndRecording();

    void        BeginFrame();
    FrameRecord EndFrame();

    int  BeginScope(const Descriptor& desc);
    void EndScope(int handle);

private:
    GpuProfiler()  = default;
    ~GpuProfiler() = default;

    std::unique_ptr<IGpuProfilerBackend> backend_;
    bool                  is_recording_            = false;
    bool                  is_frame_started_        = false;
    std::atomic<bool>     pending_begin_recording_ = false;
};

// -------------------------------------------------
// GpuScopeTimer 
// -------------------------------------------------
class GpuScopeTimer {
public:
    explicit GpuScopeTimer(const Descriptor& desc) {
        handle_ = GpuProfiler::GetInstance().BeginScope(desc);
    }

    ~GpuScopeTimer() {
        GpuProfiler::GetInstance().EndScope(handle_);
    }

    GpuScopeTimer(const GpuScopeTimer&)            = delete;
    GpuScopeTimer& operator=(const GpuScopeTimer&) = delete;
    GpuScopeTimer(GpuScopeTimer&&)                 = delete;
    GpuScopeTimer& operator=(GpuScopeTimer&&)      = delete;

private:
    int handle_ = -1;
};

} // namespace insight
