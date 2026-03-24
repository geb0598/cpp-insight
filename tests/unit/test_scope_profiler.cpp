#include <gtest/gtest.h>

#include "insight/insight.h"

namespace {

INSIGHT_DECLARE_STATGROUP("Test", STATGROUP_TEST);

INSIGHT_DECLARE_CYCLE_STAT("Outer", STAT_OUTER, STATGROUP_TEST);
INSIGHT_DECLARE_CYCLE_STAT("Inner", STAT_INNER, STATGROUP_TEST);

class ScopeProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        insight::ScopeProfiler::GetInstance().Clear();
    }
};

TEST_F(ScopeProfilerTest, FrameRecordCount) {
    INSIGHT_FRAME_BEGIN();
    {
        INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_INNER);
        }
    }
    auto frame = insight::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_EQ(frame.size(), 3u);
}

TEST_F(ScopeProfilerTest, DepthTracking) {
    INSIGHT_FRAME_BEGIN();
    {
        INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
        {
            INSIGHT_SCOPE_CYCLE_COUNTER(STAT_INNER);
        }
    }
    auto frame = insight::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_EQ(frame.size(),   3u);
    EXPECT_EQ(frame[0].depth, 2); 
    EXPECT_EQ(frame[1].depth, 1); 
    EXPECT_EQ(frame[2].depth, 0);
    EXPECT_EQ(frame[2].id,    insight::Descriptor::FRAME_ID);
}

TEST_F(ScopeProfilerTest, DurationIsPositive) {
    INSIGHT_FRAME_BEGIN();
    {
        INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
    }
    auto frame = insight::ScopeProfiler::GetInstance().EndFrame();

    EXPECT_GT(frame[0].duration.count(), 0);
}

TEST_F(ScopeProfilerTest, FrameClearedOnBeginFrame) {
    INSIGHT_FRAME_BEGIN();
    {
        INSIGHT_SCOPE_CYCLE_COUNTER(STAT_OUTER);
    }
    INSIGHT_FRAME_END();

    INSIGHT_FRAME_BEGIN();
    auto frame = insight::ScopeProfiler::GetInstance().EndFrame();
    EXPECT_EQ(frame.size(), 1u);
    EXPECT_EQ(frame[0].id, insight::Descriptor::FRAME_ID);
}

} // namespace