#include <gtest/gtest.h>

#include "insights/insights.h"

namespace {

INSIGHTS_DECLARE_STATGROUP("Test", STATGROUP_TEST);

INSIGHTS_DECLARE_STAT("Outer", STAT_OUTER, STATGROUP_TEST);
INSIGHTS_DECLARE_STAT("Inner", STAT_INNER, STATGROUP_TEST);

class ScopeProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        insights::ScopeProfiler::GetInstance().Clear();
        insights::ScopeProfiler::GetInstance().BeginRecording();
    }
};

TEST_F(ScopeProfilerTest, FrameRecordCount) {
    INSIGHTS_FRAME_BEGIN();
    {
        INSIGHTS_SCOPE(STAT_OUTER);
        {
            INSIGHTS_SCOPE(STAT_INNER);
        }
    }
    auto frame = insights::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_EQ(frame.size(), 3u);
}

TEST_F(ScopeProfilerTest, DepthTracking) {
    INSIGHTS_FRAME_BEGIN();
    {
        INSIGHTS_SCOPE(STAT_OUTER);
        {
            INSIGHTS_SCOPE(STAT_INNER);
        }
    }
    auto frame = insights::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_EQ(frame.size(),   3u);
    EXPECT_EQ(frame[0].depth, 2); 
    EXPECT_EQ(frame[1].depth, 1); 
    EXPECT_EQ(frame[2].depth, 0);
    EXPECT_EQ(frame[2].id,    insights::Descriptor::FRAME_ID);
}

TEST_F(ScopeProfilerTest, DurationIsPositive) {
    INSIGHTS_FRAME_BEGIN();
    {
        INSIGHTS_SCOPE(STAT_OUTER);
    }
    auto frame = insights::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_GT(frame[0].end_ns - frame[0].start_ns, 0);
}

TEST_F(ScopeProfilerTest, FrameClearedOnBeginFrame) {
    INSIGHTS_FRAME_BEGIN();
    {
        INSIGHTS_SCOPE(STAT_OUTER);
    }
    INSIGHTS_FRAME_END();

    INSIGHTS_FRAME_BEGIN();
    auto frame = insights::ScopeProfiler::GetInstance().EndFrame();
    EXPECT_EQ(frame.size(), 1u);
    EXPECT_EQ(frame[0].id, insights::Descriptor::FRAME_ID);
}

} // namespace