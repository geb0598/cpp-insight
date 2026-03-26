#pragma once

#include <vector>

#include "panel.h"

namespace insights::viewer {

// -------------------------------------------------
// Timeline Panel
// -------------------------------------------------
class TimelinePanel : public Panel {
public:
    TimelinePanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
    void Reset()  override;

private:
    bool reset_view_    = true;
    bool was_recording_ = false;
};

} // namespace insightss::viewer