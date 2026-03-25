#include <algorithm>
#include <atomic>

#include "insight/gpu/d3d11_gpu_profiler_backend.h"

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
        device_->CreateQuery(&disjoint_desc,  &slot.disjoint_q);
        device_->CreateQuery(&timestamp_desc, &slot.frame_start_q);
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
        if (slot.frame_start_q) {
            slot.frame_start_q->Release();
        }
        for (auto& scope : slot.scopes) {
            if (scope.begin_q) {
                scope.begin_q->Release();
            }
            if (scope.end_q) {
                scope.end_q->Release();
            }
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
}

void D3D11GpuProfilerBackend::BeginFrame(int64_t cpu_ref_ns) {
    auto& slot       = slots_[write_idx_];
    slot.scope_count = 0;
    slot.scope_depth = 0;
    slot.cpu_ref_ns  = cpu_ref_ns;
    slot.active      = true;

    context_->Begin(slot.disjoint_q);
    context_->End(slot.frame_start_q);  
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

    UINT64 frame_start_tick = 0;
    hr = context_->GetData(
        slot.frame_start_q, &frame_start_tick, sizeof(frame_start_tick), D3D11_ASYNC_GETDATA_DONOTFLUSH);
    if (hr != S_OK) {
        return {};
    }

    double freq      = static_cast<double>(disjoint.Frequency);
    double offset_ns = static_cast<double>(slot.cpu_ref_ns) - (frame_start_tick / freq * 1e9);

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

        int64_t start_ns = static_cast<int64_t>(begin_tick / freq * 1e9 + offset_ns);
        int64_t end_ns   = static_cast<int64_t>(end_tick   / freq * 1e9 + offset_ns);
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
