#pragma once

#include "ITaskBase.hpp"
#include "TTaskScheduler.hpp" 

inline void ITaskBase::Execute() {
    {
        std::lock_guard<std::mutex> lock(taskMtx_);
        if (executed_) return;
        if (started_) return;
        started_ = true;
    }

    try {
        RunWork();
    } catch (...) {
        eptr_ = std::current_exception();
    }

    {
        std::lock_guard<std::mutex> lock(taskMtx_);
        executed_ = true;
    }
    
    taskCV_.notify_all();
    std::vector<ITaskBase*> succ_copy;
    {
        std::lock_guard<std::mutex> lock(taskMtx_);
        succ_copy = successors_;
    }

    for (auto* s : succ_copy) {
        if (s->DecrementDependences()) {
            if (scheduler_ptr_) {
                scheduler_ptr_->SubmitReadyTask(s);
            }
        }
    }
}