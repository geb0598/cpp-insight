// tests/src/test_reporter.cpp
#include <gtest/gtest.h>
#include <thread>
#include "insight/insight.h"
#include "insight/reporter.h"

namespace {

INSIGHT_DECLARE_STATGROUP("Physics", STATGROUP_PHYSICS);
INSIGHT_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);
INSIGHT_DECLARE_CYCLE_STAT("Outer", STAT_OUTER, STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("Inner", STAT_INNER, STATGROUP_PHYSICS);
INSIGHT_DECLARE_CYCLE_STAT("Draw",  STAT_DRAW,  STATGROUP_RENDERING);

class ReporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        insight::ScopeProfiler::GetInstance().Clear();
        insight::Reporter::GetInstance().Clear();
    }
};

TEST_F(ReporterTest, SubmitIncreasesSize) {
    INSIGHT_FRAME_BEGIN();
    {
        INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
    }
    insight::Reporter::GetInstance().Submit(insight::ScopeProfiler::GetInstance().EndFrame());

    EXPECT_EQ(insight::Reporter::GetInstance().Size(), 1u);
}

TEST_F(ReporterTest, SummarizeByGroupReturnsCorrectGroups) {
    for (int i = 0; i < 5; ++i) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
            {
                INSIGHT_SCOPE_CYCLE_COUNTER(STAT_INNER);
            }
        }
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_DRAW);
        }
        insight::Reporter::GetInstance().Submit(insight::ScopeProfiler::GetInstance().EndFrame());
    }

    auto groups = insight::Reporter::GetInstance().SummarizeByGroup(5);

    EXPECT_EQ(groups.size(), 3u);  

    for (const auto& group : groups) {
        if (group.group_id == STATGROUP_PHYSICS.GetId()) {
            EXPECT_EQ(group.stats.size(), 2u);  
        } else if (group.group_id == STATGROUP_RENDERING.GetId()) {
            EXPECT_EQ(group.stats.size(), 1u);  
        } else if (group.group_id == insight::Group::FRAME_ID) {
            EXPECT_EQ(group.stats.size(), 1u);  
        }
    }
}

TEST_F(ReporterTest, SummarizeByGroupTimingIsPositive) {
    for (int i = 0; i < 5; ++i) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
        }
        insight::Reporter::GetInstance().Submit(insight::ScopeProfiler::GetInstance().EndFrame());
    }

    auto groups = insight::Reporter::GetInstance().SummarizeByGroup(5);
    ASSERT_FALSE(groups.empty());

    for (const auto& group : groups) {
        for (const auto& stat : group.stats) {
            if (stat.id == STAT_OUTER.GetId()) {
                EXPECT_GT(stat.timing.avg_ms, 0.0);
                EXPECT_GT(stat.timing.max_ms, 0.0);
            }
        }
    }
}

TEST_F(ReporterTest, SummarizeByStackInclusiveGeExclusive) {
    for (int i = 0; i < 5; ++i) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
            {
                INSIGHT_SCOPE_CYCLE_COUNTER(STAT_INNER);
            }
        }
        insight::Reporter::GetInstance().Submit(insight::ScopeProfiler::GetInstance().EndFrame());
    }

    auto stacks = insight::Reporter::GetInstance().SummarizeByStack(0, 5);
    ASSERT_FALSE(stacks.empty());

    for (const auto& stack : stacks) {
        if (stack.id == STAT_OUTER.GetId()) {
            EXPECT_GE(stack.inclusive.avg_ms, stack.exclusive.avg_ms);
        }
        if (stack.id == STAT_INNER.GetId()) {
            EXPECT_NEAR(stack.inclusive.avg_ms, stack.exclusive.avg_ms, 0.001);
        }
    }
}

TEST_F(ReporterTest, SummarizeByStackDepth) {
    for (int i = 0; i < 5; ++i) {
        INSIGHT_FRAME_BEGIN();
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
            {
                INSIGHT_SCOPE_CYCLE_COUNTER(STAT_INNER);
            }
        }
        insight::Reporter::GetInstance().Submit(insight::ScopeProfiler::GetInstance().EndFrame());
    }

    auto stacks = insight::Reporter::GetInstance().SummarizeByStack(0, 5);

    for (const auto& stack : stacks) {
        if (stack.id == STAT_OUTER.GetId()) {
            EXPECT_EQ(stack.depth, 1);
        }
        if (stack.id == STAT_INNER.GetId()) {
            EXPECT_EQ(stack.depth, 2);
        }
    }
}

} // namespace