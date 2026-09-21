#include <gtest/gtest.h>

#include "TTaskScheduler.hpp"
#include "TTask.hpp"
#include "TFuture.hpp"

TEST(ScedulerTest, SingleTask) {
    TTaskScheduler scheduler;

    auto t = scheduler.Add([](int a, int b) { return a + b; }, 2, 3);
    scheduler.ExecuteAll();

    EXPECT_EQ(t.GetFutureResult<int>().Get(), 5);
}

TEST(ScedulerTest, TwoIndependentTasks) {
    TTaskScheduler scheduler;

    auto t1 = scheduler.Add([](int a, int b) { return a + b; }, 2, 3);
    auto t2 = scheduler.Add([](int a, int b) {return a + b;}, 5, 7);
    scheduler.ExecuteAll();

    EXPECT_EQ(t1.GetFutureResult<int>().Get(), 5);
    EXPECT_EQ(t2.GetFutureResult<int>().Get(), 12);
} 

TEST(ScedulerTest, TwoDependingTasks) {
    TTaskScheduler scheduler;

    auto t1 = scheduler.Add([](int a, int b) { return a + b; }, 2, 3);
    auto t2 = scheduler.Add([](int a, int b) {return a + b;}, 2,
        t1.GetFutureResult<int>() 
    );
    scheduler.ExecuteAll();

    EXPECT_EQ(t1.GetFutureResult<int>().Get(), 5);
    EXPECT_EQ(t2.GetFutureResult<int>().Get(), 7);
}

TEST(SchedulerTest, ThreeTasks) {
    TTaskScheduler scheduler;

    auto t0 = scheduler.Add([](){return 10;});
    auto t1 = scheduler.Add([](int a, int b) { return a + b; }, 2,
        t0.GetFutureResult<int>() 
    );
    auto t2 = scheduler.Add([](int a, int b) {return a + b;}, 3,
        t0.GetFutureResult<int>() 
    );
    scheduler.ExecuteAll();

    EXPECT_EQ(t1.GetFutureResult<int>().Get(), 12);
    EXPECT_EQ(t2.GetFutureResult<int>().Get(), 13);
}


