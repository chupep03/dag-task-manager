#include <gtest/gtest.h>

#include "TTaskScheduler.hpp"
#include "TTask.hpp"
#include "TFuture.hpp"

TEST(CycleTest, DiamondDagDoesNotThrow) {
    TTaskScheduler scheduler;

    auto a = scheduler.Add([] { return 1; });
    auto b = scheduler.Add([](int x) { return x + 1; }, a.GetFutureResult<int>());
    auto c = scheduler.Add([](int x) { return x + 2; }, a.GetFutureResult<int>());
    auto d = scheduler.Add([](int x, int y) { return x + y; },
        b.GetFutureResult<int>(),
        c.GetFutureResult<int>());

    ASSERT_NO_THROW(scheduler.ExecuteAll());
    EXPECT_EQ(d.GetFutureResult<int>().Get(), 5);
}

TEST(CycleTest, SharedAncestorDoesNotThrow) {
    TTaskScheduler scheduler;

    auto root = scheduler.Add([] { return 10; });
    auto left  = scheduler.Add([](int x) { return x * 2; }, root.GetFutureResult<int>());
    auto right = scheduler.Add([](int x) { return x * 3; }, root.GetFutureResult<int>());
    auto join  = scheduler.Add([](int a, int b) { return a + b; },
                               left.GetFutureResult<int>(),
                               right.GetFutureResult<int>());

    ASSERT_NO_THROW(scheduler.ExecuteAll());
    EXPECT_EQ(join.GetFutureResult<int>().Get(), 50);
}

TEST(CycleTest, RepeatedExecuteAllIsOk) {
    TTaskScheduler scheduler;

    auto a = scheduler.Add([] { return 1; });
    auto b = scheduler.Add([](int x) { return x + 1; }, a.GetFutureResult<int>());

    ASSERT_NO_THROW(scheduler.ExecuteAll());
    EXPECT_EQ(b.GetFutureResult<int>().Get(), 2);

    ASSERT_NO_THROW(scheduler.ExecuteAll());
}