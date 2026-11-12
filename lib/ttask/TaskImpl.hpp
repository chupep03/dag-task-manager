#pragma once

#include <memory>
#include <vector>

#include "TaskWithResult.hpp"
#include "../callable/ICallable.hpp"

template<typename T>
class TaskImpl : public TaskWithResult<T> {
private:
    std::unique_ptr<ICallable> callable_;
    std::vector<ITaskBase*> deps_;

public:
    TaskImpl(std::unique_ptr<ICallable> cal, std::vector<ITaskBase*> deps) 
        : callable_(std::move(cal)), deps_(std::move(deps)) {}

    void RunWork() override {
        if (callable_) callable_->Run(); 
    }

    void SetCallable(std::unique_ptr<ICallable> call) {
         callable_ = std::move(call);
    }

    std::vector<ITaskBase*> GetDependences() const override {
        return deps_; 
    }
};