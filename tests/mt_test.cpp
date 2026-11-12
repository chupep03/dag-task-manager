#include <gtest/gtest.h>

#include "TTaskScheduler.hpp"
#include "TTask.hpp"
#include "TFuture.hpp"

TEST(MultithreadingTest, IndependentTasksRunInParallel) {
    TTaskScheduler scheduler;

    int N = 8;
    int sleep_ms = 200;

    std::vector<TTask> tasks;
    for (int i = 0; i < N; ++i) {
        tasks.push_back(scheduler.Add([sleep_ms](int x) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            return x * x;
        }, i));
    }

    auto start = std::chrono::steady_clock::now();
    scheduler.ExecuteAll();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();

    for (int i = 0; i < N; ++i) {
        EXPECT_EQ(tasks[i].GetFutureResult<int>().Get(), i * i);
    }

    EXPECT_LT(elapsed, 1500)
        << "Задачи выполнялись последовательно (" << elapsed
        << " мс). Ожидалось параллельное выполнение.";
}

TEST(MultithreadingTest, ManyTasksAllComplete) {
    TTaskScheduler scheduler;

    int N = 100;
    std::atomic<int> counter{0};

    std::vector<TTask> tasks;
    for (int i = 0; i < N; ++i) {
        tasks.push_back(scheduler.Add([&counter] {
            counter.fetch_add(1);
            return 42;
        }));
    }

    scheduler.ExecuteAll();

    EXPECT_EQ(counter.load(), N) << "Не все задачи выполнились";

    for (auto& t : tasks) {
        EXPECT_EQ(t.GetFutureResult<int>().Get(), 42);
    }
}

TEST(MultithreadingTest, DependenciesPreserveOrder) {
    TTaskScheduler scheduler;

    std::atomic<int> done_a{0};
    std::atomic<int> done_b{0};

    auto a = scheduler.Add([&done_a] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        done_a.store(1);
        return 10;
    });

    auto b = scheduler.Add([&done_b] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        done_b.store(1);
        return 20;
    });

    auto c = scheduler.Add([&done_a, &done_b](int x, int y) {
        EXPECT_EQ(done_a.load(), 1);
        EXPECT_EQ(done_b.load(), 1);
        return x + y;
    }, a.GetFutureResult<int>(), b.GetFutureResult<int>());

    scheduler.ExecuteAll();
    EXPECT_EQ(c.GetFutureResult<int>().Get(), 30);
}

TEST(MultithreadingTest, LongChain) {
    TTaskScheduler scheduler;

    constexpr int N = 50;

    auto t = scheduler.Add([] { return 0; });
    for (int i = 0; i < N; ++i) {
        t = scheduler.Add([](int x) { return x + 1; },
                          t.GetFutureResult<int>());
    }

    scheduler.ExecuteAll();
    EXPECT_EQ(t.GetFutureResult<int>().Get(), N);
}

TEST(MultithreadingTest, MultipleChainsInParallel) {
    TTaskScheduler scheduler;

    int chains = 4;
    int chain_len = 5;

    std::vector<TTask> ends;
    for (int c = 0; c < chains; ++c) {
        auto t = scheduler.Add([] { return 0; });
        for (int i = 0; i < chain_len; ++i) {
            t = scheduler.Add([](int x) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                return x + 1;
            }, t.GetFutureResult<int>());
        }
        ends.push_back(t);
    }

    scheduler.ExecuteAll();

    for (auto& e : ends) {
        EXPECT_EQ(e.GetFutureResult<int>().Get(), chain_len);
    }
}