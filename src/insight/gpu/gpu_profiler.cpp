#include "insight/gpu/gpu_profiler.h"
#include "insight/platform_time.h"

namespace insight {

void GpuProfiler::Init(std::unique_ptr<IGpuProfilerBackend> backend) {
    backend_ = std::move(backend);
}

void GpuProfiler::BeginRecording() {
    if (!backend_) {
        return;
    }
    is_recording_     = true;
    is_frame_started_ = false;
    backend_->BeginRecording();
}

void GpuProfiler::EndRecording() {
    is_recording_     = false;
    is_frame_started_ = false;
}

void GpuProfiler::BeginFrame() {
    if (!is_recording_ || !backend_) {
        return;
    }
    is_frame_started_ = true;

    auto    recording_start = ScopeProfiler::GetInstance().GetRecordingStart();
    int64_t cpu_ref_ns      = PlatformTime::Elapsed(recording_start, PlatformTime::Now()).count();
    backend_->BeginFrame(cpu_ref_ns);
}

FrameRecord GpuProfiler::EndFrame() {
    if (!is_recording_ || !backend_ || !is_frame_started_) {
        return {};
    }
    backend_->EndFrame();
    is_frame_started_ = false;

    return backend_->CollectFrame();
}

int GpuProfiler::BeginScope(const Descriptor& desc) {
    if (!is_frame_started_ || !backend_) {
        return -1;
    }
    return backend_->BeginScope(desc);
}

void GpuProfiler::EndScope(int handle) {
    if (!is_frame_started_ || !backend_) {
        return;
    }
    backend_->EndScope(handle);
}

} // namespace insight
