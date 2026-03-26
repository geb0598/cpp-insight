#pragma once

#include <string>

#include "panel.h"
#include "insights/server.h"

namespace insights::viewer {

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

} // namespace insightss::viewer