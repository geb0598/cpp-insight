#include <algorithm>
#include <imgui.h>

#include "insights/registry.h"
#include "insights/reporter.h"

#include "realtime_panel.h"

namespace insights::viewer {

void RealtimePanel::Render() {
    auto& reporter = Reporter::GetInstance();
    auto& registry = Registry::GetInstance();

    ImGui::BeginChild("Realtime", ImVec2(0, ImGui::GetContentRegionAvail().y * 0.32f), true);

    auto groups = reporter.SummarizeByGroup(SAMPLE_COUNT);
    std::stable_partition(groups.begin(), groups.end(),
        [](const GroupSummary& g) { return g.group_id == Group::FRAME_ID; });

    for (const auto& group : groups) {
        auto* g = registry.FindGroup(group.group_id);
        if (!g) {
            continue;
        }

        if (ImGui::CollapsingHeader(g->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            
            static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody;
            
            std::string table_id = "StatTable_" + std::to_string(g->GetId());

            if (ImGui::BeginTable(table_id.c_str(), 3, flags)) {
                ImGui::TableSetupColumn("Stat Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableHeadersRow(); 

                for (const auto& stat : group.stats) {
                    auto* d = registry.FindDescriptor(stat.id);
                    if (!d) continue;

                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", d->GetName().c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%.3f", stat.timing.avg_ms);

                    ImGui::TableNextColumn();
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%.3f", stat.timing.max_ms); 
                }
                ImGui::EndTable();
            }
        }
    }

    ImGui::EndChild();
}

} // namespace insightss::viewer