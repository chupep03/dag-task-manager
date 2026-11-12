#pragma once

#include <memory>

#include "ITaskBase.hpp"

// Override all parents methods?

template<typename T>
class TaskWithResult : public ITaskBase {
private:
    std::shared_ptr<T> result_;

public:
    TaskWithResult() : result_(nullptr) {}

    const T& GetResultRef() const {
        if (!executed_) throw std::runtime_error("Result is not ready");
        return *result_;
    }

    T&& GetResultMove() {
        if (!executed_) throw std::runtime_error("Result is not ready");
        invalidated_ = true;
        return std::move(*result_);
    }

    std::shared_ptr<T>& GetResultStorage() {return result_;}

    bool IsInvalidated() const override { return invalidated_; }
    bool IsExecuted() const override { return executed_; } 
};