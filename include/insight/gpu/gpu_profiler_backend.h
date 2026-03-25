#pragma once

#include <vector>

#include "insight/registry.h"
#include "insight/scope_profiler.h"

namespace insight {

// -------------------------------------------------
// IGpuProfilerBackend
// -------------------------------------------------
class IGpuProfilerBackend {
public:
    virtual ~IGpuProfilerBackend() = default;

    virtual void BeginRecording()                   = 0;
    virtual void BeginFrame(int64_t cpu_ref_ns)     = 0;
    virtual void EndFrame()                         = 0;
    virtual int  BeginScope(const Descriptor& desc) = 0;
    virtual void EndScope(int handle)               = 0;

    virtual std::vector<ScopeRecord> CollectFrame() = 0;
};

} // namespace insight
