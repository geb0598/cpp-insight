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
    bool reset_view_ = true;
};

} // namespace insight::viewer