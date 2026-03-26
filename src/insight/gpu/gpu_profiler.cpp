#include "insight/gpu/gpu_profiler.h"

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
    pending_begin_recording_.store(true, std::memory_order_release);
}

void GpuProfiler::EndRecording() {
    pending_begin_recording_.store(false, std::memory_order_relaxed);
    is_recording_     = false;
    is_frame_started_ = false;
}

void GpuProfiler::BeginFrame() {
    if (!is_recording_ || !backend_) {
        return;
    }
    if (pending_begin_recording_.load(std::memory_order_acquire)) {
        pending_begin_recording_.store(false, std::memory_order_relaxed);
        backend_->BeginRecording();
    }
    is_frame_started_ = true;
    backend_->BeginFrame();
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
