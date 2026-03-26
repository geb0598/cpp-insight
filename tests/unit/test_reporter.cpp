// tests/src/test_reporter.cpp
#include <gtest/gtest.h>
#include <thread>
#include "insights/insight.h"
#include "insights/reporter.h"

namespace {

INSIGHTS_DECLARE_STATGROUP("Physics", STATGROUP_PHYSICS);
INSIGHTS_DECLARE_STATGROUP("Rendering", STATGROUP_RENDERING);
INSIGHTS_DECLARE_STAT("Outer", STAT_OUTER, STATGROUP_PHYSICS);
INSIGHTS_DECLARE_STAT("Inner", STAT_INNER, STATGROUP_PHYSICS);
INSIGHTS_DECLARE_STAT("Draw",  STAT_DRAW,  STATGROUP_RENDERING);

class ReporterTest : public ::testing::Test {
protected:
    void SetUp() override {
        insights::ScopeProfiler::GetInstance().Clear();
        insights::ScopeProfiler::GetInstance().BeginRecording();
        insights::Reporter::GetInstance().Clear();
    }
};

TEST_F(ReporterTest, SubmitIncreasesSize) {
    INSIGHTS_FRAME_BEGIN();
    {
        INSIGHTS_SCOPE(STAT_OUTER);
    }
    insights::Reporter::GetInstance().Submit(insights::ScopeProfiler::GetInstance().EndFrame());

    EXPECT_EQ(insights::Reporter::GetInstance().Size(), 1u);
}

TEST_F(ReporterTest, SummarizeByGroupReturnsCorrectGroups) {
    for (int i = 0; i < 5; ++i) {
        INSIGHTS_FRAME_BEGIN();
        {
            INSIGHTS_SCOPE(STAT_OUTER);
            {
                INSIGHTS_SCOPE(STAT_INNER);
            }
        }
        {
            INSIGHTS_SCOPE(STAT_DRAW);
        }
        insights::Reporter::GetInstance().Submit(insights::ScopeProfiler::GetInstance().EndFrame());
    }

    auto groups = insights::Reporter::GetInstance().SummarizeByGroup(5);

    EXPECT_EQ(groups.size(), 3u);  

    for (const auto& group : groups) {
        if (group.group_id == STATGROUP_PHYSICS.GetId()) {
            EXPECT_EQ(group.stats.size(), 2u);  
        } else if (group.group_id == STATGROUP_RENDERING.GetId()) {
            EXPECT_EQ(group.stats.size(), 1u);  
        } else if (group.group_id == insights::Group::FRAME_ID) {
            EXPECT_EQ(group.stats.size(), 1u);  
        }
    }
}

TEST_F(ReporterTest, SummarizeByGroupTimingIsPositive) {
    for (int i = 0; i < 5; ++i) {
        INSIGHTS_FRAME_BEGIN();
        {
            INSIGHTS_SCOPE(STAT_OUTER);
        }
        insights::Reporter::GetInstance().Submit(insights::ScopeProfiler::GetInstance().EndFrame());
    }

    auto groups = insights::Reporter::GetInstance().SummarizeByGroup(5);
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
        INSIGHTS_FRAME_BEGIN();
        {
            INSIGHTS_SCOPE(STAT_OUTER);
            {
                INSIGHTS_SCOPE(STAT_INNER);
            }
        }
        insights::Reporter::GetInstance().Submit(insights::ScopeProfiler::GetInstance().EndFrame());
    }

    auto stacks = insights::Reporter::GetInstance().SummarizeByStack(0, 5);
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
        INSIGHTS_FRAME_BEGIN();
        {
            INSIGHTS_SCOPE(STAT_OUTER);
            {
                INSIGHTS_SCOPE(STAT_INNER);
            }
        }
        insights::Reporter::GetInstance().Submit(insights::ScopeProfiler::GetInstance().EndFrame());
    }

    auto stacks = insights::Reporter::GetInstance().SummarizeByStack(0, 5);

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