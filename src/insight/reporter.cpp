#include <algorithm>
#include <cassert>
#include <numeric>
#include <unordered_map>

#include "insight/reporter.h"
#include "insight/registry.h"

namespace insight {

void Reporter::Submit(FrameRecord frame) {
    if (frame.empty()) {
        return;
    }
    uint32_t track_id  = frame.front().track_id;
    auto     shared    = std::make_shared<const FrameRecord>(std::move(frame));
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_[track_id].push_back(std::move(shared));
}

void Reporter::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    tracks_.clear();
}

size_t Reporter::Size(uint32_t track_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(track_id);
    return it != tracks_.end() ? it->second.size() : 0;
}

std::vector<SharedFrame> Reporter::GetTrack(uint32_t track_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tracks_.find(track_id);
    assert(it != tracks_.end());
    return it->second;
}

size_t Reporter::TotalSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t total = 0;
    for (const auto& [id, frames] : tracks_) {
        total += frames.size();
    }
    return total;
}

bool Reporter::HasTrack(uint32_t track_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tracks_.find(track_id) != tracks_.end();
}

std::vector<GroupSummary> Reporter::SummarizeByGroup(size_t count, uint32_t track_id) const {
    std::vector<SharedFrame> frames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tracks_.find(track_id);
        if (it == tracks_.end() || it->second.empty()) {
            return {};
        }
        const auto& all = it->second;
        count = std::min(count, all.size());
        frames.assign(all.end() - static_cast<std::ptrdiff_t>(count), all.end());
    }

    std::unordered_map<Descriptor::Id, std::vector<double>> samples;
    for (const auto& frame : frames) {
        for (const auto& record : *frame) {
            samples[record.id].push_back(
                PlatformTime::ToMilli(PlatformTime::Duration(record.end_ns - record.start_ns))
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

std::vector<StackSummary> Reporter::SummarizeByStack(size_t begin, size_t end, uint32_t track_id) const {
    std::vector<SharedFrame> frames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tracks_.find(track_id);
        if (it == tracks_.end() || it->second.empty() || begin >= it->second.size()) {
            return {};
        }
        const auto& all = it->second;
        end = std::min(end, all.size());
        frames.assign(
            all.begin() + static_cast<std::ptrdiff_t>(begin),
            all.begin() + static_cast<std::ptrdiff_t>(end));
    }

    std::unordered_map<
        Descriptor::Id,
        std::unordered_map<Descriptor::Id, SampleData>> samples;

    for (const auto& frame : frames) {
        std::vector<ActiveNode> active_nodes;
        active_nodes.push_back({
            Descriptor::INVALID_ID, Descriptor::INVALID_ID, -1, 0.0, 0.0
        });

        for (int j = static_cast<int>(frame->size()) - 1; j >= 0; --j) {
            const auto& record = (*frame)[j];

            double inclusive_ms = PlatformTime::ToMilli(PlatformTime::Duration(record.end_ns - record.start_ns));

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

TimelineSummary Reporter::GetTimelineSummary(uint32_t track_id) const {
    std::vector<SharedFrame> frames;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tracks_.find(track_id);
        if (it == tracks_.end() || it->second.empty()) {
            return {};
        }
        frames = it->second;
    }

    TimelineSummary summary;
    size_t frame_count = frames.size();

    summary.total_frame_ms.resize(frame_count, 0.0f);
    summary.unaccounted_ms.resize(frame_count, 0.0f);

    for (size_t i = 0; i < frame_count; ++i) {
        for (const auto& record : *frames[i]) {
            if (record.depth == 1) {
                if (summary.tracks.find(record.id) == summary.tracks.end()) {
                    summary.tracks[record.id].resize(frame_count, 0.0f);
                }
            }
        }
    }

    for (size_t i = 0; i < frame_count; ++i) {
        double frame_total = 0.0;
        double sum         = 0.0;

        for (const auto& record : *frames[i]) {
            if (record.id == Descriptor::FRAME_ID) {
                frame_total = PlatformTime::ToMilli(PlatformTime::Duration(record.end_ns - record.start_ns));
                summary.total_frame_ms[i] = static_cast<float>(frame_total);
            } else if (record.depth == 1) {
                double ms = PlatformTime::ToMilli(PlatformTime::Duration(record.end_ns - record.start_ns));
                summary.tracks[record.id][i] += static_cast<float>(ms);
                sum += ms;
            }
        }

        summary.unaccounted_ms[i] = static_cast<float>(std::max(0.0, frame_total - sum));
    }

    return summary;
}

FlameSummary Reporter::GetFlameSummary() const {
    std::unordered_map<uint32_t, std::vector<SharedFrame>> snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        snapshot = tracks_;
    }

    FlameSummary summary;
    std::unordered_map<uint32_t, FlameTrack> track_map;

    for (const auto& [track_id, frames] : snapshot) {
        for (const auto& frame : frames) {
            for (const auto& record : *frame) {
                auto& track = track_map[record.track_id];
                if (track.scopes.empty()) {
                    if (record.track_id >= TrackId::GPU_BASE) {
                        track.label = "GPU #" + std::to_string(record.track_id - TrackId::GPU_BASE);
                    } else {
                        track.label = "CPU #" + std::to_string(record.track_id);
                    }
                    track.track_id  = record.track_id;
                    track.max_depth = 0;
                }
                track.max_depth = std::max(track.max_depth, record.depth);

                double start_ms = PlatformTime::ToMilli(PlatformTime::Duration(record.start_ns));
                double end_ms   = PlatformTime::ToMilli(PlatformTime::Duration(record.end_ns));
                track.scopes.push_back({ record.id, start_ms, end_ms, record.depth });

                summary.total_ms = std::max(summary.total_ms, end_ms);
            }
        }
    }

    summary.tracks.reserve(track_map.size());
    for (auto& [id, track] : track_map) {
        summary.tracks.push_back(std::move(track));
    }

    std::sort(summary.tracks.begin(), summary.tracks.end(), [](const FlameTrack& lhs, const FlameTrack& rhs) {
        return lhs.track_id < rhs.track_id;
    });

    return summary;
}

TimingSummary Reporter::ComputeTiming(std::vector<double> ms_values) const {
    if (ms_values.empty()) {
        return {};
    }

    std::sort(ms_values.begin(), ms_values.end());

    size_t count = ms_values.size();
    double sum   = std::accumulate(ms_values.begin(), ms_values.end(), 0.0);

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
