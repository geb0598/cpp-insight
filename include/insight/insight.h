#pragma once

// Core
#include "insight/client.h"
#include "insight/scope_profiler.h"
#include "insight/registry.h"

// GPU (Windows only)
#if defined(_WIN32)
#include "insight/gpu/gpu_profiler.h"
#endif

// -------------------------------------------------
// Stat declaration macros
// -------------------------------------------------

#define INSIGHT_DECLARE_STATGROUP(display_name, variable_name) \
    static insight::Group variable_name(display_name);

#define INSIGHT_DECLARE_CYCLE_STAT(display_name, variable_name, group) \
    static insight::Descriptor variable_name(display_name, group);

// -------------------------------------------------
// Profiling macros
// -------------------------------------------------
#define INSIGHT_SCOPE_CYCLE_COUNTER(stat) \
    insight::ScopeTimer _scope_timer_##__COUNTER__(stat)

#define INSIGHT_FRAME_BEGIN() \
    insight::ScopeProfiler::GetInstance().BeginFrame()

#define INSIGHT_FRAME_END() \
    insight::Client::GetInstance().SendFrame(insight::ScopeProfiler::GetInstance().EndFrame())

// -------------------------------------------------
// Client macros
// -------------------------------------------------
#define INSIGHT_INITIALIZE() \
    insight::Client::GetInstance().Connect()

#define INSIGHT_SHUTDOWN() \
    insight::Client::GetInstance().Disconnect()