#define NOMINMAX
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

#include <imgui.h>
#include <implot.h>

#include "insight/platform_time.h"
#include "insight/reporter.h"
#include "insight/registry.h"
#include "insight/scope_profiler.h"

#include "flame_panel.h"

namespace insight::viewer {

namespace {

ImU32 GetColorFromId(Descriptor::Id id) {
    float hue = static_cast<float>(id) * 0.618033988749895f;
    hue -= std::floor(hue);
    float r, g, b;
    ImGui::ColorConvertHSVtoRGB(hue, 0.65f, 0.85f, r, g, b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 0.9f));
}

void DrawTrack(const FlameTrack& track, int y_base, ImDrawList* dl) {
    auto& registry = Registry::GetInstance();
 
    const ImU32 border_col        = IM_COL32(0, 0, 0, 100);
    const ImU32 text_col          = IM_COL32(255, 255, 255, 220);
    const float min_px_for_text   = 30.0f;
    const float min_px_for_border = 3.0f;
 
    for (const auto& scope : track.scopes) {
        double plot_y_top = static_cast<double>(y_base + scope.depth);
        double plot_y_bot = plot_y_top + 1.0;
 
        ImVec2 p_min = ImPlot::PlotToPixels(scope.start_ms, plot_y_top);
        ImVec2 p_max = ImPlot::PlotToPixels(scope.end_ms,   plot_y_bot);
 
        if (p_min.y > p_max.y) std::swap(p_min.y, p_max.y);
 
        float width_px = p_max.x - p_min.x;
        if (width_px < 0.5f) continue;
 
        dl->AddRectFilled(p_min, p_max, GetColorFromId(scope.id));
 
        if (width_px >= min_px_for_border) {
            dl->AddRect(p_min, p_max, border_col);
        }
 
        if (width_px >= min_px_for_text) {
            auto* d = registry.FindDescriptor(scope.id);
            if (d) {
                dl->PushClipRect(p_min, p_max, true);
                dl->AddText(ImVec2(p_min.x + 3.0f, p_min.y + 2.0f), text_col,
                            d->GetName().c_str());
                dl->PopClipRect();
            }
        }
 
        if (ImGui::IsMouseHoveringRect(p_min, p_max)) {
            auto* d = registry.FindDescriptor(scope.id);
            ImGui::BeginTooltip();
            ImGui::Text("%s", d ? d->GetName().c_str() : "Unknown");
            ImGui::Text("%.4f ms  (start: %.4f ms)",
                        scope.end_ms - scope.start_ms,
                        scope.start_ms);
            ImGui::Text("depth: %d  |  track: %u", scope.depth, track.track_id);
            ImGui::EndTooltip();
        }
    }
}

} // namespace

void FlamePanel::Render() {
    auto& reporter = Reporter::GetInstance();
 
    size_t current_count = reporter.Size();
    if (current_count != cached_frame_count_) {
        cached_summary_     = reporter.GetFlameSummary();
        cached_frame_count_ = current_count;
    }
 
    if (cached_summary_.tracks.empty()) {
        ImGui::BeginChild("FlameGraph", ImVec2(0, 150), true);
        ImGui::TextDisabled("Waiting for profile data...");
        ImGui::EndChild();
        return;
    }
 
    std::vector<int> y_bases;
    int y_cursor = 0;
    for (const auto& track : cached_summary_.tracks) {
        y_bases.push_back(y_cursor);
        y_cursor += track.max_depth + 2;
    }
    int total_rows = y_cursor;
 
    const float row_height_px = 20.0f;
    float avail_h  = ImGui::GetContentRegionAvail().y;
    double y_range = std::max(static_cast<double>(total_rows) + 1.0,
                              static_cast<double>(avail_h / row_height_px));

    ImGui::BeginChild("FlameGraph", ImVec2(0, 0), true);

    if (!ImPlot::BeginPlot("##Flame", ImVec2(-1, -1),
                           ImPlotFlags_NoLegend | ImPlotFlags_NoMenus)) {
        ImGui::EndChild();
        return;
    }
 
    ImPlot::SetupAxes("Time (s)", nullptr,
                      ImPlotAxisFlags_None,
                      ImPlotAxisFlags_Invert       |
                      ImPlotAxisFlags_NoTickLabels |
                      ImPlotAxisFlags_NoGridLines  |
                      ImPlotAxisFlags_Lock);
    ImPlot::SetupAxisFormat(ImAxis_X1, [](double ms, char* buf, int size, void*) -> int {
        return snprintf(buf, size, "%.2fs", ms * 1e-3);
    }, nullptr);

    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, cached_summary_.total_ms, ImPlotCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -0.5, y_range, ImPlotCond_Always);
 
    for (size_t i = 0; i < cached_summary_.tracks.size(); ++i) {
        ImPlot::Annotation(0.0, static_cast<double>(y_bases[i]),
                           ImVec4(0.8f, 0.8f, 0.8f, 1.0f),
                           ImVec2(4, 0), false,
                           "%s", cached_summary_.tracks[i].label.c_str());
    }
 
    ImPlot::PushPlotClipRect();
    ImDrawList* dl = ImPlot::GetPlotDrawList();
 
    const ImU32 sep_col = IM_COL32(80, 80, 80, 180);
    for (size_t i = 1; i < cached_summary_.tracks.size(); ++i) {
        double   sep_y   = static_cast<double>(y_bases[i]) - 0.5;
        ImVec2   p_left  = ImPlot::PlotToPixels(0.0,                       sep_y);
        ImVec2   p_right = ImPlot::PlotToPixels(cached_summary_.total_ms,  sep_y);
        dl->AddLine(p_left, p_right, sep_col, 1.0f);
    }
 
    for (size_t i = 0; i < cached_summary_.tracks.size(); ++i) {
        DrawTrack(cached_summary_.tracks[i], y_bases[i], dl);
    }
 
    ImPlot::PopPlotClipRect();
    ImPlot::EndPlot();
    ImGui::EndChild();
}

void FlamePanel::Reset() {
    cached_summary_     = {};
    cached_frame_count_ = 0;
}

} // namespace insight::viewer
