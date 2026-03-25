// tests/src/test_stat_registry.cpp
#include <gtest/gtest.h>
#include "insight/insight.h"

namespace {

INSIGHT_DECLARE_STATGROUP("Physics",   STATGROUP_PHYSICS);
INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);

INSIGHT_DECLARE_STAT("BVH Traverse", STAT_BVH,   STATGROUP_PHYSICS);
INSIGHT_DECLARE_STAT("Broad Phase",  STAT_BROAD, STATGROUP_PHYSICS);
INSIGHT_DECLARE_STAT("Draw Call",    STAT_DRAW,  STATGROUP_RENDERING);

class StatRegistryTest : public ::testing::Test {
protected:
    void TearDown() override {
        auto& registry = insight::Registry::GetInstance();
        registry.Clear();
        registry.RegisterGroup(&STATGROUP_PHYSICS);
        registry.RegisterGroup(&STATGROUP_RENDERING);
        registry.RegisterDescriptor(&STAT_BVH);
        registry.RegisterDescriptor(&STAT_BROAD);
        registry.RegisterDescriptor(&STAT_DRAW);
    }
};

TEST_F(StatRegistryTest, GroupRegistered) {
    auto* group = insight::Registry::GetInstance()
                      .FindGroup(STATGROUP_PHYSICS.GetId());
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->GetName(), "Physics");
}

TEST_F(StatRegistryTest, DescriptorRegistered) {
    auto* desc = insight::Registry::GetInstance()
                     .FindDescriptor(STAT_BVH.GetId());
    ASSERT_NE(desc, nullptr);
    EXPECT_EQ(desc->GetName(), "BVH Traverse");
}

TEST_F(StatRegistryTest, GroupContainsDescriptors) {
    auto* group = insight::Registry::GetInstance()
                      .FindGroup(STATGROUP_PHYSICS.GetId());
    ASSERT_NE(group, nullptr);
    EXPECT_EQ(group->GetDescriptors().size(), 2u);
}

TEST_F(StatRegistryTest, SerializeAndDeserialize) {
    auto& registry = insight::Registry::GetInstance();
    auto& groups   = registry.GetGroups();
    auto& descs    = registry.GetDescriptors();

    int32_t original_group_count = static_cast<int32_t>(groups.size());
    int32_t original_desc_count  = static_cast<int32_t>(descs.size());

    insight::BinaryWriter writer;
    writer << original_group_count;
    for (auto& [id, group] : groups) { writer << *group; }
    writer << original_desc_count;
    for (auto& [id, desc] : descs)   { writer << *desc; }

    registry.Clear();

    insight::BinaryReader reader(writer.GetBuffer());

    std::vector<std::unique_ptr<insight::Group>>      owned_groups;
    std::vector<std::unique_ptr<insight::Descriptor>> owned_descs;

    int32_t g_count;
    reader << g_count;
    for (int32_t i = 0; i < g_count; ++i) {
        auto group = std::make_unique<insight::Group>();
        reader << *group;
        registry.RegisterGroup(group.get());
        owned_groups.push_back(std::move(group));
    }

    int32_t d_count;
    reader << d_count;
    for (int32_t i = 0; i < d_count; ++i) {
        auto desc = std::make_unique<insight::Descriptor>();
        reader << *desc;
        registry.RegisterDescriptor(desc.get());
        owned_descs.push_back(std::move(desc));
    }

    EXPECT_EQ(static_cast<int32_t>(registry.GetGroups().size()),      original_group_count);
    EXPECT_EQ(static_cast<int32_t>(registry.GetDescriptors().size()), original_desc_count);

    auto* physics = registry.FindGroup(STATGROUP_PHYSICS.GetId());
    ASSERT_NE(physics, nullptr);
    EXPECT_EQ(physics->GetName(), "Physics");
}

} // namespace