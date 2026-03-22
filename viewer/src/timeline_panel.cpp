#include <imgui.h>

#include "insight/reporter.h"
#include "insight/scope_profiler.h"

#include "timeline_panel.h"

namespace insight::viewer {

void TimelinePanel::Render() {
    auto& context = GetContext();
    auto& reporter = Reporter::GetInstance();
    const auto& frames = reporter.GetFrames();

    for (size_t i = last_processed_; i < frames.size(); ++i) {
        for (const auto& record : frames[i]) {
            if (record.id == Descriptor::FRAME_ID) {
                frame_times_.push_back(static_cast<float>(
                    PlatformTime::ToMilli(record.duration)));
                break;
            }
        }
    } 
    last_processed_ = frames.size();

    ImGui::BeginChild("Timeline",
        ImVec2(0, ImGui::GetContentRegionAvail().y * 0.35f),
        true);

    if (!frame_times_.empty()) {
        ImGui::PlotLines("##timeline",
            frame_times_.data(),
            static_cast<int>(frame_times_.size()),
            0, nullptr, 0.0f, 33.0f,
            ImVec2(ImGui::GetContentRegionAvail().x, 80.0f));
    }

    int total = static_cast<int>(frame_times_.size());
    int begin = static_cast<int>(context.timeline_begin);
    int end   = static_cast<int>(context.timeline_end);

    ImGui::SliderInt("Begin", &begin, 0, std::max(0, total - 1));
    ImGui::SliderInt("End", &end, begin, std::max(0, total - 1));

    context.timeline_begin = static_cast<size_t>(begin);
    context.timeline_end = static_cast<size_t>(end);

    ImGui::EndChild();
    ImGui::Separator();
}

void TimelinePanel::Reset() {
    frame_times_.clear();
    last_processed_ = 0;
}

} // namespace insight::viewer