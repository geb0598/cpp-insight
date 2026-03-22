#include <imgui.h>

#include "insight/reporter.h"
#include "insight/registry.h"

#include "stack_panel.h"

namespace insight::viewer {

void StackPanel::Render() {
    auto& context  = GetContext();
    auto& reporter = Reporter::GetInstance();
    auto& registry = Registry::GetInstance();

    if (context.timeline_begin != last_begin_ ||
        context.timeline_end   != last_end_) {
        if (context.timeline_begin < context.timeline_end) {
            cached_ = reporter.SummarizeByStack(
                context.timeline_begin,
                context.timeline_end);
        }
        last_begin_ = context.timeline_begin;
        last_end_   = context.timeline_end;
    }

    ImGui::BeginChild("Stack", ImVec2(0, 0), true);

    ImGui::Columns(4, "stack_cols", true);
    ImGui::Text("Name");     ImGui::NextColumn();
    ImGui::Text("Incl.Avg"); ImGui::NextColumn();
    ImGui::Text("Excl.Avg"); ImGui::NextColumn();
    ImGui::Text("Incl.Max"); ImGui::NextColumn();
    ImGui::Separator();

    for (const auto& stack : cached_) {
        auto* d = registry.FindDescriptor(stack.id); 
        if (!d) {
            continue;
        }

        std::string indent(static_cast<size_t>(stack.depth) * 2, ' ');
        std::string name = indent + d->GetName();

        ImGui::Text("%s", name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%.3fms", stack.inclusive.avg_ms);
        ImGui::NextColumn();
        ImGui::Text("%.3fms", stack.exclusive.avg_ms);
        ImGui::NextColumn();
        ImGui::Text("%.3fms", stack.inclusive.max_ms);
        ImGui::NextColumn();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void StackPanel::Reset() {
    cached_.clear();
    last_begin_ = 0;
    last_end_   = 0;
}

} // namespace insight::viewer