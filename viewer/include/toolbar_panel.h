#pragma once

#include <string>

#include "panel.h"
#include "insight/server.h"

namespace insight::viewer {

// -------------------------------------------------
// ToolbarPanel
// -------------------------------------------------
class ToolbarPanel : public Panel {
public:
    ToolbarPanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;

private:
    std::string status_message_;
};

} // namespace insight::viewer