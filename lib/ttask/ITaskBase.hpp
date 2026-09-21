#pragma once

#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

class TTaskScheduler;

class ITaskBase {
protected:
    std::atomic<int> deps_remaining_;
    std::vector<ITaskBase*> successors_;
    mutable std::mutex taskMtx_;
    std::condition_variable taskCV_;

    TTaskScheduler* scheduler_ptr_;
    std::exception_ptr eptr_;

    bool started_ = false;
    bool executed_ = false;
    bool invalidated_ = false;

public:
    virtual void RunWork() = 0;
    void Execute();

    bool DecrementDependences() {return --deps_remaining_ == 0;}
    void AddSuccessor(ITaskBase* t) { 
        successors_.push_back(t);
        t->deps_remaining_++;
    } 

    void SetRemainingDeps(size_t sz) {
        deps_remaining_ = static_cast<int>(sz);
    }
    void SetSchedulerPtr(TTaskScheduler* ptr) {scheduler_ptr_ = ptr; }

    int GetRemainingDeps() const {return deps_remaining_.load();}
    std::mutex& GetMutex() {return taskMtx_;}
    std::condition_variable& GetCV() {return taskCV_;}
    std::exception_ptr GetEptr() {return eptr_;}

    
    bool IsStartedButNotFinished() const {
        std::lock_guard<std::mutex> lock(taskMtx_);        
        return started_ && !executed_;
    }

     void RethrowIfFailed() const {
        if (eptr_) std::rethrow_exception(eptr_);
    }

    virtual bool IsExecuted() const = 0;
    virtual bool IsInvalidated() const = 0;
    virtual std::vector<ITaskBase*> GetDependences() const = 0;
    virtual ~ITaskBase() = default;
}; 
// #include "ITaskBase.tpp"   