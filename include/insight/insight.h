#pragma once

// Core
#include "insight/archive.h"
#include "insight/platform_time.h"
#include "insight/scope_profiler.h"
// #include "insight/stat_aggregator.h"
#include "insight/stat_registry.h"

// GPU (Windows only)
#if defined(INSIGHT_PLATFORM_WINDOWS)
#include "insight/gpu/gpu_profiler.h"
#endif

// -------------------------------------------------
// Stat declaration macros
// -------------------------------------------------

#define INSIGHT_DECLARE_STATGROUP(display_name, variable_name) \
    static insight::StatGroup variable_name(display_name);

#define INSIGHT_DECLARE_CYCLE_STAT(display_name, variable_name, group) \
    static insight::StatDescriptor variable_name(display_name, group);

// -------------------------------------------------
// Profiling macros
// -------------------------------------------------
#define INSIGHT_SCOPE_CYCLE_COUNTER(stat) \
    insight::ScopeTimer _scope_timer_##__LINE__(stat)

#define INSIGHT_FRAME_BEGIN() \
    insight::ScopeProfiler::GetInstance().BeginFrame()

#define INSIGHT_FRAME_END() \
    insight::ScopeProfiler::GetInstance().EndFrame()

// -------------------------------------------------
// Session macros
// -------------------------------------------------
#define INSIGHT_SESSION_START() \
    insight::StatAggregator::GetInstance().StartSession()

#define INSIGHT_SESSION_STOP() \
    insight::StatAggregator::GetInstance().StopSession()