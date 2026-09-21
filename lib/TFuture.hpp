#pragma once

#include <memory>
#include <ttask/TaskWithResult.hpp>

template<typename T>
class TFuture {
private:
    using DecayT = std::decay_t<T>;

    std::shared_ptr<TaskWithResult<DecayT>> task_;
    //bool is_ref_ = std::is_reference<T>::value; 
    std::atomic<bool> moved_;

public:
    TFuture(std::shared_ptr<TaskWithResult<DecayT>> t) : task_(t), moved_(false) {}
    TFuture(TFuture<T>&& other) : task_(std::move(other.task_)), moved_(other.moved_.load()) {}
    TFuture(TFuture<T>& other) = delete;
    
    TFuture& operator = (TFuture<T>&& other) {
        task_ = std::move(other.task_);
        moved_.store(other.moved_.load());
        return *this;
    }
    TFuture& operator = (TFuture<T>& other) = delete;

    T Get()  {
        if (!task_) throw std::runtime_error("Future isnt initializated");
        {
            std::unique_lock<std::mutex> lock(task_->GetMutex());
            task_->GetCV().wait(lock, [this](){return task_->IsExecuted();});
            task_->RethrowIfFailed();
        }
        if constexpr (std::is_reference_v<T>) {
            return task_->GetResultRef();
        } else {
            if (moved_.exchange(true)) {
                throw std::runtime_error("Result has been already moved from this TFuture object");
            }
        }
        return task_->GetResultMove();
    }

    ITaskBase* GetTaskBase() const {
        return static_cast<ITaskBase*>(task_.get());
    }
    
    bool IsReady() const {
        if (!task_) return false;
        std::lock_guard<std::mutex> lock(task_->taskMtx_);
        return task_->executed_;
    }
};