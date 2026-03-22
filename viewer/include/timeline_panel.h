#pragma once

#include <vector>

#include "panel.h"

namespace insight::viewer {

// -------------------------------------------------
// Timeline Panel
// -------------------------------------------------
class TimelinePanel : public Panel {
public:
    TimelinePanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
    void Reset()  override;

private:
    std::vector<float> frame_times_;
    size_t             last_processed_ = 0;
};

} // namespace insight::viewer