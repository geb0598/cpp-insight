#pragma once

#include "insight/profile_types.h"

#include "panel.h"

namespace insight::viewer {

// -------------------------------------------------
// Flame Graph Panel
// -------------------------------------------------
class FlamePanel : public Panel {
public:
    FlamePanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
    void Reset()  override;

private:
    FlameSummary cached_summary_;
    size_t       cached_frame_count_ = 0;
};

} // namespace insight::viewer
