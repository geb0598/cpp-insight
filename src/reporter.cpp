#include <algorithm>
#include <numeric>
#include <unordered_map>

#include "insight/reporter.h"
#include "insight/registry.h"

namespace insight {

void Reporter::Submit(FrameRecord frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.push_back(std::move(frame));
}

void Reporter::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.clear();
}

std::vector<GroupSummary> Reporter::SummarizeByGroup(size_t count) const {
    std::lock_guard<std::mutex> lock(mutex_);

    if (frames_.empty()) {
        return {};
    }

    count = std::min(count, frames_.size());
    auto begin = frames_.end() - static_cast<std::ptrdiff_t>(count);

    std::unordered_map<Descriptor::Id, std::vector<double>> samples;
    for (auto it = begin; it != frames_.end(); ++it) {
        for (const auto& record : *it) {
            samples[record.id].push_back(
                PlatformTime::ToMilli(record.duration)
            );
        }
    }

    std::vector<GroupSummary> result;
    const auto& groups = Registry::GetInstance().GetGroups();
    for (const auto& [group_id, group] : groups) {
        GroupSummary group_summary;
        group_summary.group_id = group_id;

        for (Descriptor::Id desc_id : group->GetDescriptors()) {
            auto it = samples.find(desc_id);
            if (it == samples.end()) {
                continue;
            }

            StatSummary stat_summary;
            stat_summary.id = desc_id;
            stat_summary.timing = ComputeTiming(std::move(it->second));
            group_summary.stats.push_back(std::move(stat_summary));
        }

        if (!group_summary.stats.empty()) {
            result.push_back(std::move(group_summary));
        }
    }

    return result;
}

namespace {
    struct SampleData {
        int                 depth;
        std::vector<double> inclusive_ms;
        std::vector<double> exclusive_ms;
    };

    struct ActiveNode {
        Descriptor::Id id;
        Descriptor::Id parent_id;
        int            depth;
        double         inclusive_ms;
        double         children_ms;
    };
}

std::vector<StackSummary> Reporter::SummarizeByStack(size_t begin, size_t end) const {
    if (frames_.empty() || begin >= frames_.size()) {
        return {};
    }

    end = std::min(end, frames_.size());

    std::unordered_map<
        Descriptor::Id, 
        std::unordered_map<Descriptor::Id, SampleData>> samples;

    for (size_t i = begin; i < end; ++i) {
        const auto& frame = frames_[i];

        std::vector<ActiveNode> active_nodes;

        active_nodes.push_back({
            Descriptor::INVALID_ID, Descriptor::INVALID_ID, -1, 0.0, 0.0
        });

        for (int j = static_cast<int>(frame.size()) - 1; j >= 0; --j) {
            const auto& record = frame[j];

            double inclusive_ms = PlatformTime::ToMilli(record.duration);

            while (active_nodes.back().depth >= record.depth) {
                auto popped = active_nodes.back();
                active_nodes.pop_back();

                double exclusive_ms = std::max(0.0, popped.inclusive_ms - popped.children_ms);

                auto& data = samples[popped.id][popped.parent_id];
                data.depth = popped.depth;
                data.inclusive_ms.push_back(popped.inclusive_ms);
                data.exclusive_ms.push_back(exclusive_ms);
            }

            Descriptor::Id parent_id = active_nodes.back().id;

            active_nodes.back().children_ms += inclusive_ms;

            active_nodes.push_back({
                record.id, parent_id, record.depth, inclusive_ms, 0.0
            });
        }

        while (active_nodes.size() > 1) {
            auto popped = active_nodes.back();
            active_nodes.pop_back();

            double exclusive_ms = std::max(0.0, popped.inclusive_ms - popped.children_ms);

            auto& data = samples[popped.id][popped.parent_id];
            data.depth = popped.depth;
            data.inclusive_ms.push_back(popped.inclusive_ms);
            data.exclusive_ms.push_back(exclusive_ms);
        }
    }

    std::vector<StackSummary> result;
    for (auto& [id, parent_map] : samples) {
        for (auto& [parent_id, data] : parent_map) {
            StackSummary summary;
            summary.id        = id;
            summary.parent_id = parent_id;
            summary.depth     = data.depth;
            summary.count     = data.inclusive_ms.size();
            summary.inclusive = ComputeTiming(std::move(data.inclusive_ms));
            summary.exclusive = ComputeTiming(std::move(data.exclusive_ms));
            result.push_back(std::move(summary));
        }
    }

    return result;
}

TimelineSummary Reporter::GetTimelineSummary() const {
    std::lock_guard<std::mutex> lock(mutex_);
    TimelineSummary summary;
    size_t frame_count = frames_.size();

    if (frame_count == 0) {
        return summary;
    }

    summary.total_frame_ms.resize(frame_count, 0.0f);
    summary.unaccounted_ms.resize(frame_count, 0.0f);

    for (size_t i = 0; i < frame_count; ++i) {
        for (const auto& record : frames_[i]) {
            if (record.depth == 1) {
                if (summary.tracks.find(record.id) == summary.tracks.end()) {
                    summary.tracks[record.id].resize(frame_count, 0.0f);
                }
            }
        }
    }

    for (size_t i = 0; i < frame_count; ++i) {
        double frame_total = 0.0;
        double sum = 0.0;

        for (const auto& record : frames_[i]) {
            if (record.id == Descriptor::FRAME_ID) {
                frame_total = PlatformTime::ToMilli(record.duration);
                summary.total_frame_ms[i] = static_cast<float>(frame_total);
            } else if (record.depth == 1) {
                double ms = PlatformTime::ToMilli(record.duration);
                summary.tracks[record.id][i] += static_cast<float>(ms);
                sum += ms;
            }
        }

        summary.unaccounted_ms[i] = static_cast<float>(std::max(0.0, frame_total - sum));
    }

    return summary;
}

TimingSummary Reporter::ComputeTiming(std::vector<double> ms_values) const {
    if (ms_values.empty()) {
        return {};
    }

    std::sort(ms_values.begin(), ms_values.end());

    size_t count = ms_values.size();
    double sum = std::accumulate(ms_values.begin(), ms_values.end(), 0.0);

    TimingSummary timing;
    timing.avg_ms = sum / count;
    timing.min_ms = ms_values.front();
    timing.max_ms = ms_values.back();

    if (count >= 20) {
        timing.p95_ms = ms_values[static_cast<size_t>(count * 0.95)];
        timing.p99_ms = ms_values[static_cast<size_t>(count * 0.99)];
    }

    return timing;
}

} // namespace insight