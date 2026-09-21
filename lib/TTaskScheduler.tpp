#pragma once

#include "TTaskScheduler.hpp"
#include "TTask.hpp"

template<typename F, typename... Args>
TTask TTaskScheduler::Add(F func, Args&&... args) {

    std::vector<ITaskBase*> dependences;
    auto collect_one = [&](auto& arg) {
        using TArg = std::decay_t<decltype(arg)>;
        if constexpr (detail::is_future_helper<TArg>::value) {
            dependences.push_back(arg.GetTaskBase());
        }
    };
    (collect_one(args), ...);

    auto wrapper = [f = std::forward<F>(func), args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
        return std::apply([&f](auto&&... unpacked_args) {
            return f(detail::Get(unpacked_args)...);
        }, args_tuple);
    };

    using ReturnT = std::invoke_result_t<decltype(wrapper)>;
    using DecayT  = std::decay_t<ReturnT>;  
    
    auto task_ptr = std::make_shared<TaskImpl<DecayT>>(nullptr, dependences);
    auto callable_impl = std::make_unique<CallableImpl<decltype(wrapper)>>(
        std::move(wrapper), 
        task_ptr->GetResultStorage()
    );

    task_ptr->SetCallable(std::move(callable_impl));

    {
        std::lock_guard<std::mutex> lock(this->queueMtx_);
        //task_ptr->SetRemainingDeps(dependences.size());
        task_ptr->SetSchedulerPtr(this);

        for(auto* dep : dependences) {
            std::lock_guard<std::mutex> tasklock(dep->GetMutex());
            dep->AddSuccessor(task_ptr.get()); 
        }
        this->tasks_.push_back(task_ptr);
    }

    return TTask(task_ptr, this); 
}

#include "ttask/ITaskBase.tpp"