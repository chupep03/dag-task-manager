#pragma once

#include <memory>
#include <type_traits>
#include <stdexcept>

#include "ttask/ITaskBase.hpp"

class TTaskScheduler;
template <typename T> class TFuture;
template <typename T> class TaskWithResult;

class TTask {
private:
    std::shared_ptr<ITaskBase> impl_;
    TTaskScheduler* scheduler_;

public:
    TTask(std::shared_ptr<ITaskBase> i, TTaskScheduler* s)
        : impl_(std::move(i)), scheduler_(s) {}

    // Result of processing here and now
    template <typename T>
    T GetResultSync();

    // Promise that there will be result
    template <typename T>
    TFuture<T> GetFutureResult();

    // Add dependent task
    template <typename F>
    TTask Apply(F&& f);
};

#include "TTask.tpp"