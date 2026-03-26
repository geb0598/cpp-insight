#pragma once

#include <vector>

#include "insights/registry.h"
#include "insights/scope_profiler.h"

namespace insights {

// -------------------------------------------------
// IGpuProfilerBackend
// -------------------------------------------------
class IGpuProfilerBackend {
public:
    virtual ~IGpuProfilerBackend() = default;

    virtual void BeginRecording()                   = 0;
    virtual void BeginFrame()                       = 0;
    virtual void EndFrame()                         = 0;
    virtual int  BeginScope(const Descriptor& desc) = 0;
    virtual void EndScope(int handle)               = 0;

    virtual std::vector<ScopeRecord> CollectFrame() = 0;
};

} // namespace insights
