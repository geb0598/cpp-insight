#pragma once

#include <vector>

#include "insight/profile_types.h"

#include "panel.h"

namespace insight::viewer {

// -------------------------------------------------
// Stack Panel
// -------------------------------------------------
class StackPanel : public Panel {
public:
    StackPanel(PanelContext& ctx) : Panel(ctx) {}

    void Render() override;
    void Reset()  override;

private:
    void DrawNode(const StackSummary* node, 
                  const std::unordered_map<Descriptor::Id, std::vector<const StackSummary*>>& tree);

    std::vector<insight::StackSummary> cached_;
    size_t                             last_begin_ = 0;
    size_t                             last_end_ = 0;
};

} // namespace insight::viewer