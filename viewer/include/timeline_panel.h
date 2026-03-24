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
};

} // namespace insight::viewer