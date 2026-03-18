#include <thread>
#include <cassert>
#include <iostream>

#include "insight/platform_time.h"
#include "insight/stat_registry.h"
#include "insight/archive.h"
#include "insight/scope_profiler.h"

INSIGHT_DECLARE_STATGROUP("Physics", STATGROUP_PHYSICS);
INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);
INSIGHT_DECLARE_CYCLE_STAT("BVH Traverse",  STAT_BVH,      STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("Broad Phase",   STAT_BROAD,    STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("Draw Call",     STAT_DRAW,     STATGROUP_RENDERING);

void TestStatSerialization() {
    auto& registry = insight::StatRegistry::GetInstance();

    insight::BinaryWriter writer;

    auto& groups = registry.GetGroups();
    auto& descs  = registry.GetDescriptors();

    int32_t group_count = static_cast<int32_t>(groups.size());
    writer << group_count;
    for (auto& [id, group] : groups) {
        writer << *group;
    }

    int32_t desc_count = static_cast<int32_t>(descs.size());
    writer << desc_count;
    for (auto& [id, desc] : descs) {
        writer << *desc;
    }

    std::vector<std::pair<insight::StatGroup::Id, std::string>> original_groups;
    for (auto& [id, group] : groups) {
        original_groups.push_back({ id, group->GetName() });
    }
    std::vector<std::pair<insight::StatDescriptor::Id, std::string>> original_descs;
    for (auto& [id, desc] : descs) {
        original_descs.push_back({ id, desc->GetName() });
    }

    registry.Clear();
    assert(registry.GetGroups().empty());
    assert(registry.GetDescriptors().empty());

    insight::BinaryReader reader(writer.GetBuffer());

    std::vector<std::unique_ptr<insight::StatGroup>>      owned_groups;
    std::vector<std::unique_ptr<insight::StatDescriptor>> owned_descs;

    int32_t g_count;
    reader << g_count;
    for (int32_t i = 0; i < g_count; ++i) {
        auto group = std::make_unique<insight::StatGroup>();
        reader << *group;
        registry.RegisterGroup(group.get());
        owned_groups.push_back(std::move(group));
    }

    int32_t d_count;
    reader << d_count;
    for (int32_t i = 0; i < d_count; ++i) {
        auto desc = std::make_unique<insight::StatDescriptor>();
        reader << *desc;
        registry.RegisterDescriptor(desc.get());
        owned_descs.push_back(std::move(desc));
    }

    for (auto& [id, name] : original_groups) {
        auto* group = registry.FindGroup(id);
        assert(group != nullptr);
        assert(group->GetName() == name);
        std::cout << "Group OK: " << name << "\n";
    }

    for (auto& [id, name] : original_descs) {
        auto* desc = registry.FindDescriptor(id);
        assert(desc != nullptr);
        assert(desc->GetName() == name);
        std::cout << "Descriptor OK: " << name << "\n";
    }
}

void TestFrameSerialization() {
    insight::ScopeProfiler::GetInstance().BeginFrame();
    insight::ScopeProfiler::GetInstance().BeginScope(STAT_BVH);
    insight::ScopeProfiler::GetInstance().BeginScope(STAT_BROAD);
    insight::ScopeProfiler::GetInstance().EndScope();
    insight::ScopeProfiler::GetInstance().EndScope();
    auto frame = insight::ScopeProfiler::GetInstance().EndFrame();

    insight::BinaryWriter writer;
    writer << frame;

    insight::BinaryReader reader(writer.GetBuffer());
    insight::FrameRecord  restored;
    reader << restored;

    assert(frame.size() == restored.size());
    for (size_t i = 0; i < frame.size(); ++i) {
        assert(frame[i].id       == restored[i].id);
        assert(frame[i].duration == restored[i].duration);
        assert(frame[i].depth    == restored[i].depth);
        std::cout << "ScopeRecord OK: id=" << restored[i].id
                  << " depth=" << restored[i].depth << "\n";
    }
}

int main() {
    using namespace insight;

    auto start = PlatformTime::Now();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    auto end = PlatformTime::Now();

    auto elapsed = PlatformTime::Elapsed(start, end);

    std::cout << "Elapsed (ms):  " << PlatformTime::ToMilli(elapsed)  << "\n";
    std::cout << "Elapsed (us):  " << PlatformTime::ToMicro(elapsed)  << "\n";
    std::cout << "Elapsed (sec): " << PlatformTime::ToSeconds(elapsed) << "\n";

    // ---

    std::cout << "=== Test 1: Stat Serialization ===\n";
    TestStatSerialization();

    std::cout << "\n=== Test 2: Frame Serialization ===\n";
    TestFrameSerialization();

    std::cout << "\nAll tests passed.\n";
    return 0;
}