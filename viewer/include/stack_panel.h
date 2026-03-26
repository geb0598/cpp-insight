#pragma once

#include <vector>

#include "insights/profile_types.h"

#include "panel.h"

namespace insights::viewer {

// -------------------------------------------------
// Stack Panel
// -------------------------------------------------
class StackPanel : public Panel {
public:
    StackPanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
    void Reset()  override;

private:
    struct CachedTrack {
        std::vector<insights::StackSummary> data;
        size_t last_begin      = 0;
        size_t last_end        = 0;
        size_t last_track_size = 0;
    };

    void DrawNode(const StackSummary* node,
                  const std::unordered_map<Descriptor::Id, std::vector<const StackSummary*>>& tree);

    void DrawTrackSection(uint32_t track_id, size_t begin, size_t end,
                          CachedTrack& cache, const char* table_id, bool is_recording);

    CachedTrack cpu_cache_;
    CachedTrack gpu_cache_;
};

} // namespace insightss::viewer
