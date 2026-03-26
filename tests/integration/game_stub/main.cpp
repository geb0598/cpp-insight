#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "insight/insight.h"
#include "insight/gpu/gpu_profiler.h"
#include "insight/gpu/gpu_profiler_backend.h"

void BusyWaitMicroseconds(int micro_sec) {
    auto start = std::chrono::high_resolution_clock::now();
    auto target = start + std::chrono::microseconds(micro_sec);
    while (std::chrono::high_resolution_clock::now() < target) {
    }
}

// -------------------------------------------------
// MockGpuProfilerBackend
// -------------------------------------------------
class MockGpuProfilerBackend : public insight::IGpuProfilerBackend {
public:
    static constexpr int     FRAME_LATENCY         = 3;
    static constexpr int     MAX_SCOPES_PER_FRAME  = 64;
    static constexpr int64_t GPU_LATENCY_NS        = 2'000'000;

    struct ScopeEntry {
        insight::Descriptor::Id id       = insight::Descriptor::INVALID_ID;
        int                     depth    = 0;
        int64_t                 begin_ns = 0;
        int64_t                 end_ns   = 0;
    };

    struct FrameSlot {
        bool       active      = false;
        int        scope_count = 0;
        int        scope_depth = 0;
        ScopeEntry scopes[MAX_SCOPES_PER_FRAME];
    };

    MockGpuProfilerBackend() : track_id_(AllocTrackId()) {}

    void BeginRecording() override {
        write_idx_    = 0;
        frame_num_    = 0;
        base_raw_ns_  = NowNs();
        for (auto& slot : slots_) {
            slot.active      = false;
            slot.scope_count = 0;
            slot.scope_depth = 0;
        }
    }

    void BeginFrame() override {
        auto& slot       = slots_[write_idx_];
        slot.active      = true;
        slot.scope_count = 0;
        slot.scope_depth = 0;
    }

    void EndFrame() override {
        write_idx_ = (write_idx_ + 1) % FRAME_LATENCY;
        ++frame_num_;
    }

    int BeginScope(const insight::Descriptor& desc) override {
        auto& slot = slots_[write_idx_];
        if (slot.scope_count >= MAX_SCOPES_PER_FRAME) {
            return -1;
        }
        int   handle   = slot.scope_count++;
        auto& entry    = slot.scopes[handle];
        entry.id       = desc.GetId();
        entry.depth    = slot.scope_depth++;
        entry.begin_ns = NowNs();
        return handle;
    }

    void EndScope(int handle) override {
        if (handle < 0) {
            return;
        }
        auto& slot = slots_[write_idx_];
        --slot.scope_depth;
        slot.scopes[handle].end_ns = NowNs();
    }

    std::vector<insight::ScopeRecord> CollectFrame() override {
        if (frame_num_ < FRAME_LATENCY) {
            return {};
        }
        auto& slot = slots_[write_idx_];
        if (!slot.active) {
            return {};
        }

        int64_t jitter_ns = (std::rand() % 500'000) - 250'000;

        std::vector<insight::ScopeRecord> results;
        results.reserve(slot.scope_count);
        for (int i = 0; i < slot.scope_count; ++i) {
            const auto& e   = slot.scopes[i];
            // Convert epoch-absolute CPU timestamps to recording-relative,
            // then add a simulated GPU queue latency.
            int64_t start_ns = static_cast<int64_t>(e.begin_ns - base_raw_ns_) + GPU_LATENCY_NS + jitter_ns;
            int64_t end_ns   = static_cast<int64_t>(e.end_ns   - base_raw_ns_) + GPU_LATENCY_NS + jitter_ns;
            results.push_back({ e.id, start_ns, end_ns, e.depth, track_id_ });
        }

        std::sort(results.begin(), results.end(),
                  [](const insight::ScopeRecord& a, const insight::ScopeRecord& b) {
                      if (a.end_ns != b.end_ns) {
                          return a.end_ns < b.end_ns;
                      }
                      return a.depth > b.depth;
                  });

        slot.active = false;
        return results;
    }

private:
    static uint32_t AllocTrackId() {
        static std::atomic<uint32_t> counter{ insight::TrackId::GPU_BASE };
        return counter.fetch_add(1);
    }

    static int64_t NowNs() {
        using namespace std::chrono;
        return duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    }

    uint32_t  track_id_;
    int64_t   base_raw_ns_ = 0;
    int       write_idx_   = 0;
    int       frame_num_   = 0;
    FrameSlot slots_[FRAME_LATENCY];
};

// -------------------------------------------------
// Stats
// -------------------------------------------------
INSIGHT_DECLARE_STATGROUP("Physics",   STATGROUP_PHYSICS);
INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);
INSIGHT_DECLARE_STATGROUP("AI",        STATGROUP_AI);

INSIGHT_DECLARE_STAT("PhysicsUpdate", STAT_PHYSICS,   STATGROUP_PHYSICS);
INSIGHT_DECLARE_STAT("BVHTraverse",   STAT_BVH,       STATGROUP_PHYSICS);
INSIGHT_DECLARE_STAT("Collision",     STAT_COLLISION, STATGROUP_PHYSICS);
INSIGHT_DECLARE_STAT("DrawCall",      STAT_DRAW,      STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("ShadowPass",    STAT_SHADOW,    STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("AIUpdate",      STAT_AI,        STATGROUP_AI);

INSIGHT_DECLARE_STAT("GpuShadow",    STAT_GPU_SHADOW, STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("GpuOpaque",    STAT_GPU_OPAQUE, STATGROUP_RENDERING);
INSIGHT_DECLARE_STAT("GpuParticles", STAT_GPU_FX,     STATGROUP_RENDERING);

int main() {
    std::cout << "Connecting to viewer...\n";

    insight::GpuProfiler::GetInstance().Init(
        std::make_unique<MockGpuProfilerBackend>());

    INSIGHT_INITIALIZE();
    std::cout << "Connect thread started. Running game loop...\n";

    int frame = 0;
    while (true) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE(STAT_PHYSICS);
            BusyWaitMicroseconds(1800);
            {
                INSIGHT_SCOPE(STAT_BVH);
                BusyWaitMicroseconds(900);
                {
                    INSIGHT_SCOPE(STAT_COLLISION);
                    BusyWaitMicroseconds(400);
                }
            }
        }
        {
            INSIGHT_SCOPE(STAT_AI);
            // Edge case: occasional AI spike (variable frame time)
            int sleep_us = (frame % 30 == 0) ? 3000 : 600;
            BusyWaitMicroseconds(sleep_us);
        }
        {
            INSIGHT_SCOPE(STAT_DRAW);
            BusyWaitMicroseconds(2500);
            {
                INSIGHT_SCOPE(STAT_SHADOW);
                BusyWaitMicroseconds(800);
            }

            {
                INSIGHT_GPU_SCOPE(STAT_GPU_SHADOW);
                BusyWaitMicroseconds(1200);
            }
            {
                INSIGHT_GPU_SCOPE(STAT_GPU_OPAQUE);
                BusyWaitMicroseconds(3500);
                {
                    INSIGHT_GPU_SCOPE(STAT_GPU_FX);
                    BusyWaitMicroseconds(600);
                }
            }
        }
        INSIGHT_FRAME_END();

        // ~60 fps target
        BusyWaitMicroseconds(3000);
        ++frame;
    }

    insight::Client::GetInstance().Disconnect();
    return 0;
}