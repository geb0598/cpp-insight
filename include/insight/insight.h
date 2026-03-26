#pragma once

// Core
#include "insight/client.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

// GPU 
#if defined(INSIGHT_GPU)
#include "insight/gpu/gpu_profiler.h"
#endif

// -------------------------------------------------
// Stat declaration macros
// -------------------------------------------------

#define INSIGHT_DECLARE_STATGROUP(display_name, variable_name) \
    static insight::Group variable_name(display_name);

#define INSIGHT_DECLARE_STAT(display_name, variable_name, group) \
    static insight::Descriptor variable_name(display_name, group);

// -------------------------------------------------
// GPU conditional helpers
// -------------------------------------------------

#if defined(INSIGHT_GPU)

#define INSIGHT_GPU_IMPL_FRAME_BEGIN() \
    insight::GpuProfiler::GetInstance().BeginFrame();

#define INSIGHT_GPU_IMPL_FRAME_END() \
    do { \
        auto _gpu_frame_ = insight::GpuProfiler::GetInstance().EndFrame(); \
        if (!_gpu_frame_.empty()) { \
            insight::Client::GetInstance().SendFrame(std::move(_gpu_frame_)); \
        } \
    } while(0)

#define INSIGHT_GPU_SCOPE(stat) \
    insight::GpuScopeTimer _gpu_scope_##__COUNTER__(stat)

#define INSIGHT_INITIALIZE() \
    do { \
        insight::ScopeProfiler::GetInstance().SetOnBeginRecording([]() { \
            insight::GpuProfiler::GetInstance().BeginRecording(); \
        }); \
        insight::ScopeProfiler::GetInstance().SetOnEndRecording([]() { \
            insight::GpuProfiler::GetInstance().EndRecording(); \
        }); \
        insight::Client::GetInstance().Connect(); \
    } while(0)

#else

#define INSIGHT_GPU_IMPL_FRAME_BEGIN()
#define INSIGHT_GPU_IMPL_FRAME_END()
#define INSIGHT_GPU_SCOPE(stat)

#define INSIGHT_INITIALIZE() \
    insight::Client::GetInstance().Connect()

#endif

// -------------------------------------------------
// Profiling macros
// -------------------------------------------------

#define INSIGHT_SCOPE(stat) \
    insight::ScopeTimer _scope_timer_##__COUNTER__(stat)

#define INSIGHT_FRAME_BEGIN() \
    do { \
        INSIGHT_GPU_IMPL_FRAME_BEGIN() \
        insight::ScopeProfiler::GetInstance().BeginFrame(); \
    } while(0)

#define INSIGHT_FRAME_END() \
    do { \
        insight::Client::GetInstance().SendFrame( \
            insight::ScopeProfiler::GetInstance().EndFrame()); \
        INSIGHT_GPU_IMPL_FRAME_END(); \
    } while(0)

// -------------------------------------------------
// Client macros
// -------------------------------------------------

#define INSIGHT_SHUTDOWN() \
    insight::Client::GetInstance().Disconnect()
