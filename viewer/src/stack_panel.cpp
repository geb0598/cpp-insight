#define NOMINMAX
#include <algorithm>

#include <imgui.h>

#include "insights/reporter.h"
#include "insights/registry.h"
#include "insights/scope_profiler.h"

#include "stack_panel.h"

namespace insights::viewer {

namespace {

static const ImGuiTableFlags TABLE_FLAGS =
    ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
    ImGuiTableFlags_RowBg    | ImGuiTableFlags_Resizable     |
    ImGuiTableFlags_NoBordersInBody;

} // namespace

void StackPanel::Render() {
    auto& context  = GetContext();
    auto& reporter = Reporter::GetInstance();

    size_t cpu_size = reporter.Size(TrackId::CPU_BASE);
    size_t gpu_size = reporter.Size(TrackId::GPU_BASE);

    size_t cpu_begin = context.timeline_begin;
    size_t cpu_end   = context.timeline_end + 1;

    // Use raw timeline values — SummarizeByStack clamps internally.
    // Clamping here caused gpu_end to always equal gpu_size when the
    // selection extends past the available GPU frame count, so the
    // GPU tab never updated when the user moved the right marker.
    size_t gpu_begin = context.timeline_begin;
    size_t gpu_end   = context.timeline_end + 1;

    ImGui::BeginChild("Stack", ImVec2(0, 0), true);

    if (ImGui::BeginTabBar("StackTabs")) {

        if (ImGui::BeginTabItem("CPU")) {
            ImGui::Text("Frame %zu ~ %zu", context.timeline_begin, context.timeline_end);
            ImGui::Separator();
            DrawTrackSection(TrackId::CPU_BASE, cpu_begin, cpu_end, cpu_cache_, "CpuStackTable");
            ImGui::EndTabItem();
        }

        if (reporter.HasTrack(TrackId::GPU_BASE)) {
            if (ImGui::BeginTabItem("GPU")) {
                ImGui::Text("Frame %zu ~ %zu  (%zu GPU frames total)",
                            gpu_begin, gpu_end > 0 ? gpu_end - 1 : 0, gpu_size);
                ImGui::Separator();
                DrawTrackSection(TrackId::GPU_BASE, gpu_begin, gpu_end, gpu_cache_, "GpuStackTable");
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();
}

void StackPanel::DrawTrackSection(uint32_t track_id, size_t begin, size_t end,
                                  CachedTrack& cache, const char* table_id) {
    auto& reporter   = Reporter::GetInstance();
    size_t track_size = reporter.Size(track_id);

    if (begin != cache.last_begin     ||
        end   != cache.last_end       ||
        track_size != cache.last_track_size) {
        if (begin < end) {
            cache.data = reporter.SummarizeByStack(begin, end, track_id);
        } else {
            cache.data.clear();
        }
        cache.last_begin      = begin;
        cache.last_end        = end;
        cache.last_track_size = track_size;
    }

    std::unordered_map<Descriptor::Id, std::vector<const StackSummary*>> tree;
    std::vector<const StackSummary*> roots;

    for (const auto& item : cache.data) {
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

    if (ImGui::BeginTable(table_id, 5, TABLE_FLAGS)) {
        ImGui::TableSetupColumn("Function / Scope", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Count",            ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Incl. Avg (ms)",  ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Excl. Avg (ms)",  ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Incl. Max (ms)",  ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();
        for (const auto* root : roots) {
            DrawNode(root, tree);
        }
        ImGui::EndTable();
    }
}

void StackPanel::Reset() {
    cpu_cache_ = {};
    gpu_cache_ = {};
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

} // namespace insightss::viewer
