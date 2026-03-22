#include <imgui.h>

#include "insight/registry.h"
#include "insight/reporter.h"

#include "realtime_panel.h"

namespace insight::viewer {

void RealtimePanel::Render() {
    auto& reporter = Reporter::GetInstance();
    auto& registry = Registry::GetInstance();

    ImGui::BeginChild("Realtime",
        ImVec2(0, ImGui::GetContentRegionAvail().y * 0.3f),
        true);

    auto groups = reporter.SummarizeByGroup(SAMPLE_COUNT);

    for (const auto& group : groups) {
        auto* g = registry.FindGroup(group.group_id);
        if (!g) {
            continue;
        }

        if (ImGui::TreeNodeEx(g->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Columns(3, "realtime_cols", true);
            ImGui::Text("Name"); ImGui::NextColumn();
            ImGui::Text("Avg");  ImGui::NextColumn();
            ImGui::Text("Max");  ImGui::NextColumn();
            ImGui::Separator();

            for (const auto& stat : group.stats) {
                auto* d = registry.FindDescriptor(stat.id);
                if (!d) {
                    continue;
                }

                ImGui::Text("%s", d->GetName().c_str());
                ImGui::NextColumn();
                ImGui::Text("%.3fms", stat.timing.avg_ms);
                ImGui::NextColumn();
                ImGui::Text("%.3fms", stat.timing.max_ms);
                ImGui::NextColumn();
            }

            ImGui::Columns(1);
            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
    ImGui::Separator();
}

} // namespace insight::viewer