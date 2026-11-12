#pragma once

#include "TTask.hpp"
#include "TTaskScheduler.hpp"
#include "TFuture.hpp"
#include "ttask/TaskWithResult.hpp"

template <typename T>
T TTask::GetResultSync() {
    using DecayedT = std::remove_cv_t<std::remove_reference_t<T>>;

    auto typed_task = std::dynamic_pointer_cast<TaskWithResult<DecayedT>>(impl_);
    if (!typed_task) {
        throw std::runtime_error("Type mismatch in GetResultSync, check the return type (T).");
    }

    if (scheduler_) {
        scheduler_->ExecuteTaskBranch(typed_task);
    }

    TFuture<T> fut(typed_task);
    return fut.Get();
}

template <typename T>
TFuture<T> TTask::GetFutureResult() {
    using DecayedT = std::decay_t<T>;

    auto typed_task = std::dynamic_pointer_cast<TaskWithResult<DecayedT>>(impl_);
    if (!typed_task) {
        throw std::runtime_error("Type mismatch: the task result type does not match T.");
    }

    return TFuture<T>(typed_task);
}

template <typename F>
TTask TTask::Apply(F&& f) {
    if (!scheduler_) {
        throw std::runtime_error("Scheduler is no longer available");
    }

    using ArgType = typename detail::function_traits<std::decay_t<F>>::arg0_type;
    return scheduler_->Add(std::forward<F>(f),
                           this->template GetFutureResult<ArgType>());
}