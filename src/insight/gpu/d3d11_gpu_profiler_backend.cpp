#include <algorithm>
#include <atomic>

#include "insight/gpu/d3d11_gpu_profiler_backend.h"
#include "insight/platform_time.h"
#include "insight/scope_profiler.h"

namespace insight {

namespace {

uint32_t AllocGpuTrackId() {
    static std::atomic<uint32_t> counter{ TrackId::GPU_BASE };
    return counter.fetch_add(1);
}

} // namespace

D3D11GpuProfilerBackend::D3D11GpuProfilerBackend(ID3D11Device* device, ID3D11DeviceContext* context)
    : device_(device), context_(context), track_id_(AllocGpuTrackId())
{
    D3D11_QUERY_DESC disjoint_desc  = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
    D3D11_QUERY_DESC timestamp_desc = { D3D11_QUERY_TIMESTAMP,          0 };

    for (auto& slot : slots_) {
        device_->CreateQuery(&disjoint_desc, &slot.disjoint_q);
        for (auto& scope : slot.scopes) {
            device_->CreateQuery(&timestamp_desc, &scope.begin_q);
            device_->CreateQuery(&timestamp_desc, &scope.end_q);
        }
    }
}

D3D11GpuProfilerBackend::~D3D11GpuProfilerBackend() {
    for (auto& slot : slots_) {
        if (slot.disjoint_q) {
            slot.disjoint_q->Release();
        }
        for (auto& scope : slot.scopes) {
            if (scope.begin_q) { scope.begin_q->Release(); }
            if (scope.end_q)   { scope.end_q->Release();   }
        }
    }
}

void D3D11GpuProfilerBackend::BeginRecording() {
    write_idx_ = 0;
    frame_num_ = 0;
    for (auto& slot : slots_) {
        slot.active      = false;
        slot.scope_count = 0;
        slot.scope_depth = 0;
    }

    ID3D11Query* calib_q = nullptr;
    ID3D11Query* disj_q  = nullptr;
    D3D11_QUERY_DESC td  = { D3D11_QUERY_TIMESTAMP,          0 };
    D3D11_QUERY_DESC dd  = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
    if (FAILED(device_->CreateQuery(&td, &calib_q))) { return; }
    if (FAILED(device_->CreateQuery(&dd, &disj_q))) {
        calib_q->Release();
        return;
    }

    {
        UINT64 dummy = 0;
        context_->End(calib_q);
        context_->Flush();
        while (context_->GetData(calib_q, &dummy, sizeof(dummy), 0) == S_FALSE) { 
            Sleep(0); 
        }
    }

    context_->Begin(disj_q);
    auto cpu_now = PlatformTime::Now();
    context_->End(calib_q);
    context_->End(disj_q);
    context_->Flush();

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
    while (context_->GetData(disj_q, &disjoint, sizeof(disjoint), 0) == S_FALSE) { 
        Sleep(0); 
    }

    UINT64 gpu_tick = 0;
    while (context_->GetData(calib_q, &gpu_tick, sizeof(gpu_tick), 0) == S_FALSE) { 
        Sleep(0); 
    }

    calib_q->Release();
    disj_q->Release();

    if (disjoint.Disjoint || disjoint.Frequency == 0) { 
        return; 
    }

    base_cpu_ns_   = 0;
    base_gpu_tick_ = gpu_tick;
}

void D3D11GpuProfilerBackend::BeginFrame() {
    auto& slot       = slots_[write_idx_];
    slot.scope_count = 0;
    slot.scope_depth = 0;
    slot.active      = true;

    context_->Begin(slot.disjoint_q);
}

void D3D11GpuProfilerBackend::EndFrame() {
    context_->End(slots_[write_idx_].disjoint_q);
    write_idx_ = (write_idx_ + 1) % FRAME_LATENCY;
    ++frame_num_;
}

int D3D11GpuProfilerBackend::BeginScope(const Descriptor& desc) {
    auto& slot = slots_[write_idx_];
    if (slot.scope_count >= MAX_SCOPES_PER_FRAME) {
        return -1;
    }

    int   handle  = slot.scope_count++;
    auto& entry   = slot.scopes[handle];
    entry.id      = desc.GetId();
    entry.depth   = slot.scope_depth++;
    context_->End(entry.begin_q);
    return handle;
}

void D3D11GpuProfilerBackend::EndScope(int handle) {
    if (handle < 0) {
        return;
    }
    auto& slot = slots_[write_idx_];
    --slot.scope_depth;
    context_->End(slot.scopes[handle].end_q);
}

std::vector<ScopeRecord> D3D11GpuProfilerBackend::CollectFrame() {
    if (frame_num_ < FRAME_LATENCY) {
        return {};
    }

    auto& slot = slots_[write_idx_];
    if (!slot.active) {
        return {};
    }

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
    HRESULT hr = context_->GetData(
        slot.disjoint_q, &disjoint, sizeof(disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH);
    if (hr != S_OK || disjoint.Disjoint) {
        return {};
    }

    std::vector<ScopeRecord> results;
    results.reserve(slot.scope_count);

    for (int i = 0; i < slot.scope_count; ++i) {
        auto& entry = slot.scopes[i];

        UINT64 begin_tick = 0;
        UINT64 end_tick   = 0;
        if (context_->GetData(entry.begin_q, &begin_tick, sizeof(begin_tick), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) {
            continue;
        }
        if (context_->GetData(entry.end_q, &end_tick, sizeof(end_tick), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK) {
            continue;
        }

        double  freq     = static_cast<double>(disjoint.Frequency);
        int64_t start_ns = base_cpu_ns_ + static_cast<int64_t>(static_cast<double>(begin_tick - base_gpu_tick_) / freq * 1e9);
        int64_t end_ns   = base_cpu_ns_ + static_cast<int64_t>(static_cast<double>(end_tick   - base_gpu_tick_) / freq * 1e9);
        results.push_back({ entry.id, start_ns, end_ns, entry.depth, track_id_ });
    }

    std::sort(results.begin(), results.end(), [](const ScopeRecord& lhs, const ScopeRecord& rhs) {
        if (lhs.end_ns != rhs.end_ns) {
            return lhs.end_ns < rhs.end_ns;
        }
        return lhs.depth > rhs.depth;
    });

    slot.active = false;
    return results;
}

} // namespace insight
