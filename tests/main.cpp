#include <iostream>
#include <thread>

#include "insight/platform_time.h"
#include "insight/stat_id.h"

INSIGHT_DECLARE_STATGROUP("Physics", STATGROUP_PHYSICS)
INSIGHT_DECLARE_CYCLE_STAT("BVH Traverse", STAT_BVH_TRAVERSE, STATGROUP_PHYSICS)
INSIGHT_DECLARE_CYCLE_STAT("Broad Phase", STAT_BROAD_PHASE, STATGROUP_PHYSICS)

INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING)
INSIGHT_DECLARE_CYCLE_STAT("Draw Call", STAT_DRAW_CALL, STATGROUP_RENDERING)

int main() {
    using namespace insight;

    auto start = PlatformTime::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    auto end = PlatformTime::Now();

    auto elapsed = PlatformTime::Elapsed(start, end);

    std::cout << "Elapsed (ms):  " << PlatformTime::ToMilli(elapsed)  << "\n";
    std::cout << "Elapsed (us):  " << PlatformTime::ToMicro(elapsed)  << "\n";
    std::cout << "Elapsed (sec): " << PlatformTime::ToSeconds(elapsed) << "\n";

    const auto& groups = insight::StatRegistry::GetInstance().GetGroups();
    const auto& stats  = insight::StatRegistry::GetInstance().GetStats();

    for (const auto& [group_id, group] : groups) {
        std::cout << "[" << group->GetName() << "]\n";
        for (const auto* stat : stats.at(group_id)) {
            std::cout << "  " << stat->GetName()
                      << " (id: " << stat->GetId() << ")\n";
        }
    }

    return 0;
}