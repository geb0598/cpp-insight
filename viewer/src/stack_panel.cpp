#include <algorithm>

#include <imgui.h>

#include "insight/reporter.h"
#include "insight/registry.h"

#include "stack_panel.h"

namespace insight::viewer {

void StackPanel::Render() {
auto& context  = GetContext();
    auto& reporter = Reporter::GetInstance();

    if (context.timeline_begin != last_begin_ || context.timeline_end != last_end_) {
        if (context.timeline_begin <= context.timeline_end) {
            cached_ = reporter.SummarizeByStack(context.timeline_begin, context.timeline_end + 1);
        }
        last_begin_ = context.timeline_begin;
        last_end_   = context.timeline_end;
    }

    ImGui::BeginChild("Stack", ImVec2(0, 0), true);
    ImGui::Text("Selected Range Details (Frame %zu ~ %zu)", context.timeline_begin, context.timeline_end);
    ImGui::Separator();

    std::unordered_map<Descriptor::Id, std::vector<const StackSummary*>> tree;
    std::vector<const StackSummary*> roots;

    for (const auto& item : cached_) {
        if (item.parent_id == Descriptor::INVALID_ID) {
            roots.push_back(&item);
        } else {
            tree[item.parent_id].push_back(&item);
        }
    }

    auto sort_desc = [](const StackSummary* a, const StackSummary* b) {
        return a->inclusive.avg_ms > b->inclusive.avg_ms;
    };
    std::sort(roots.begin(), roots.end(), sort_desc);
    for (auto& [pid, children] : tree) {
        std::sort(children.begin(), children.end(), sort_desc);
    }

    static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | 
                                   ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody;

    if (ImGui::BeginTable("StackTable", 5, flags)) {
        ImGui::TableSetupColumn("Function / Scope", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 50.0f); 
        ImGui::TableSetupColumn("Incl. Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Excl. Avg (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Incl. Max (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        for (const auto* root : roots) {
            DrawNode(root, tree);
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void StackPanel::Reset() {
    cached_.clear();
    last_begin_ = 0;
    last_end_   = 0;
}

void StackPanel::DrawNode(const StackSummary* node, 
                          const std::unordered_map<Descriptor::Id, std::vector<const StackSummary*>>& tree) {
    auto& registry = Registry::GetInstance();

    auto* d = registry.FindDescriptor(node->id);
    if (!d) {
        return;
    }

ImGui::TableNextRow();
    ImGui::TableNextColumn();

    auto it = tree.find(node->id);
    bool has_children = (it != tree.end() && !it->second.empty());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (!has_children) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet; 
    }
    if (node->depth < 2) { 
        flags |= ImGuiTreeNodeFlags_DefaultOpen; 
    }

    bool is_open = ImGui::TreeNodeEx(d->GetName().c_str(), flags);

    ImGui::TableNextColumn();
    ImGui::Text("%zu", node->count);

    ImGui::TableNextColumn();
    ImGui::Text("%.3f", node->inclusive.avg_ms);
    
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%.3f", node->exclusive.avg_ms); 
    
    ImGui::TableNextColumn();
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.6f, 1.0f), "%.3f", node->inclusive.max_ms);

    if (is_open) {
        if (has_children) {
            for (const auto* child : it->second) {
                DrawNode(child, tree);
            }
        }
        ImGui::TreePop(); 
    }
}

} // namespace insight::viewer