#pragma once

#ifdef INSIGHTS

// Core
#include "insights/client.h"
#include "insights/scope_profiler.h"
#include "insights/registry.h"

// GPU
#if defined(INSIGHTS_GPU)
#include "insights/gpu/gpu_profiler.h"
#endif

// -------------------------------------------------
// Stat declaration macros
// -------------------------------------------------

#define INSIGHTS_DECLARE_STATGROUP(display_name, variable_name) \
    static insights::Group variable_name(display_name);

#define INSIGHTS_DECLARE_STAT(display_name, variable_name, group) \
    static insights::Descriptor variable_name(display_name, group);

// -------------------------------------------------
// GPU conditional helpers
// -------------------------------------------------

#if defined(INSIGHTS_GPU)

#define INSIGHTS_GPU_INIT_D3D11(device, context) \
    insights::GpuProfiler::GetInstance().Init( \
        std::make_unique<insights::D3D11GpuProfilerBackend>(device, context))

#define INSIGHTS_GPU_IMPL_FRAME_BEGIN() \
    insights::GpuProfiler::GetInstance().BeginFrame();

#define INSIGHTS_GPU_IMPL_FRAME_END() \
    do { \
        auto _gpu_frame_ = insights::GpuProfiler::GetInstance().EndFrame(); \
        if (!_gpu_frame_.empty()) { \
            insights::Client::GetInstance().SendFrame(std::move(_gpu_frame_)); \
        } \
    } while(0)

#define INSIGHTS_GPU_SCOPE(stat) \
    insights::GpuScopeTimer _gpu_scope_##__COUNTER__(stat)

#define INSIGHTS_INITIALIZE() \
    do { \
        insights::ScopeProfiler::GetInstance().SetOnBeginRecording([]() { \
            insights::GpuProfiler::GetInstance().BeginRecording(); \
        }); \
        insights::ScopeProfiler::GetInstance().SetOnEndRecording([]() { \
            insights::GpuProfiler::GetInstance().EndRecording(); \
        }); \
        insights::Client::GetInstance().Connect(); \
    } while(0)

#else

#define INSIGHTS_GPU_INIT_D3D11(device, context)
#define INSIGHTS_GPU_IMPL_FRAME_BEGIN()
#define INSIGHTS_GPU_IMPL_FRAME_END()
#define INSIGHTS_GPU_SCOPE(stat)

#define INSIGHTS_INITIALIZE() \
    insights::Client::GetInstance().Connect()

#endif

// -------------------------------------------------
// Profiling macros
// -------------------------------------------------

#define INSIGHTS_SCOPE(stat) \
    insights::ScopeTimer _scope_timer_##__COUNTER__(stat)

#define INSIGHTS_FRAME_BEGIN() \
    do { \
        INSIGHTS_GPU_IMPL_FRAME_BEGIN() \
        insights::ScopeProfiler::GetInstance().BeginFrame(); \
    } while(0)

#define INSIGHTS_FRAME_END() \
    do { \
        insights::Client::GetInstance().SendFrame( \
            insights::ScopeProfiler::GetInstance().EndFrame()); \
        INSIGHTS_GPU_IMPL_FRAME_END(); \
    } while(0)

// -------------------------------------------------
// Client macros
// -------------------------------------------------

#define INSIGHTS_SHUTDOWN() \
    insights::Client::GetInstance().Disconnect()

#else // INSIGHTS 

#define INSIGHTS_DECLARE_STATGROUP(display_name, variable_name)
#define INSIGHTS_DECLARE_STAT(display_name, variable_name, group)
#define INSIGHTS_GPU_INIT_D3D11(device, context)
#define INSIGHTS_GPU_SCOPE(stat)
#define INSIGHTS_INITIALIZE()
#define INSIGHTS_SHUTDOWN()
#define INSIGHTS_SCOPE(stat)
#define INSIGHTS_FRAME_BEGIN()
#define INSIGHTS_FRAME_END()

#endif // INSIGHTS
