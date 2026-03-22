#pragma once

#include "panel.h"

namespace insight::viewer {

// -------------------------------------------------
// RealtimePanel 
// -------------------------------------------------
class RealtimePanel : public Panel {
public:
    static constexpr size_t SAMPLE_COUNT = 300;

    explicit RealtimePanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
};

} // namespace insight::viewer