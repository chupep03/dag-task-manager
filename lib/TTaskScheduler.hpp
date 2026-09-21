#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <queue>
#include <condition_variable>
#include <type_traits>
#include <stdexcept>
#include <unordered_map>
#include <atomic>

#include "ttask/ITaskBase.hpp"
#include "callable/CallableImpl.hpp"
#include "ttask/TaskImpl.hpp"

class ITaskBase;
template <typename T> class TFuture;
class TTask;   // forward — БЕЗ include "TTask.hpp"

namespace detail {
    template <typename T> struct is_future_helper : std::false_type {};
    template <typename T> struct is_future_helper<TFuture<T>> : std::true_type {};

    template <typename T>
    struct UnwrapFuture { using type = T; };

    template <typename T>
    struct UnwrapFuture<TFuture<T>> { using type = T; };


    // function traits
    template <typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template <typename ClassType, typename ReturnType, typename Arg>
    struct function_traits<ReturnType(ClassType::*)(Arg) const> {
        using arg0_type = Arg;
    };

    template <typename ClassType, typename ReturnType, typename Arg>
    struct function_traits<ReturnType(ClassType::*)(Arg)> {
        using arg0_type = Arg;
    };

    template <typename ReturnType, typename Arg>
    struct function_traits<ReturnType(*)(Arg)> {
        using arg0_type = Arg;
    };


    template <typename T>
    auto Get(T& arg) {
        if constexpr (is_future_helper<std::decay_t<T>>::value) {
            return arg.Get();
        } else {
            return arg;
        }
    }
}

class TTaskScheduler {
private:
    std::vector<std::shared_ptr<ITaskBase>> tasks_;
    std::queue<ITaskBase*> ready_tasks_;

    std::vector<std::thread> workers_;
    std::mutex queueMtx_;
    std::condition_variable queue_cv_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> completed_cnt_{0};
    std::atomic<size_t> total_tasks_{0};

    std::exception_ptr eptr_;

    std::mutex done_mtx_;
    std::condition_variable done_cv_;

   
public:
    TTaskScheduler() = default;
    TTaskScheduler(const TTaskScheduler& other) = delete;
    TTaskScheduler& operator = (const TTaskScheduler& other) = delete;

    template<typename F, typename... Args>
    TTask Add(F func, Args&&... args);  // in .tpp  file
    void ExecuteAll(bool single_thread = false) {
        if (single_thread) {
            return;
        }

        CheckCycles();
        completed_cnt_ = 0;
        stop_ = false;
        total_tasks_ = tasks_.size();

        int workers_num = std::thread::hardware_concurrency();
        if (workers_num < 1) workers_num = 2;

        for (int i = 0; i < workers_num; ++i) workers_.emplace_back(&TTaskScheduler::WorkerLoop, this);

        {
            std::lock_guard<std::mutex> lock(queueMtx_);
            for(auto& t : tasks_) {
                if (t->GetRemainingDeps() == 0) {
                    ready_tasks_.push(t.get());
                }
            }
        }
        queue_cv_.notify_all();

        
        std::unique_lock<std::mutex> lock(done_mtx_);
        auto predicate = [this]() {return completed_cnt_.load() == total_tasks_;};
        done_cv_.wait(lock, predicate);

        Shutdown();
    }

    void ExecuteTaskBranch(ITaskBase* target) {
        if (target->IsExecuted()) return;

        for (ITaskBase* dep : target->GetDependences()) {
            ExecuteTaskBranch(dep);
        }

        std::unique_lock<std::mutex> lock(target->GetMutex());
        if (target->IsExecuted()) return;

        if (target->IsStartedButNotFinished()) {
            auto predicate = [target]() {return target->IsExecuted();};
            target->GetCV().wait(lock, predicate);
        } else {
            lock.unlock();
            target->Execute();
        }
    }

    void SubmitReadyTask(ITaskBase* task) {
        {
            std::lock_guard<std::mutex> lock(queueMtx_);
            ready_tasks_.push(task);
        }
        queue_cv_.notify_all();
    }

    void Shutdown() {
        stop_ = true;
        queue_cv_.notify_all();
        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
        workers_.clear();
    }

    void WorkerLoop() {
        while(!stop_) {
            ITaskBase* task_ptr;
            {
                std::unique_lock<std::mutex> lock(queueMtx_);
                auto cv_predicate = [this]() {return !ready_tasks_.empty() || stop_;};
                queue_cv_.wait(lock, cv_predicate);

                if (stop_ && ready_tasks_.empty()) return;
                task_ptr = ready_tasks_.front();
                ready_tasks_.pop();
            }
            if (task_ptr) {
                task_ptr->Execute();
                size_t done = ++completed_cnt_;
                if (done == total_tasks_) {
                    std::lock_guard<std::mutex> lock(done_mtx_);
                    done_cv_.notify_all();
                }
            }
        }
    }

private:
    std::unordered_map<ITaskBase*, int> visited_;

    void dfs(ITaskBase* cur_node) {
        visited_[cur_node] = 1;
        for (auto n : cur_node->GetDependences()) {
            if (visited_[n] == 1) {
                throw std::runtime_error("Cycle detected in Task Graph!");
            }
            if (visited_[n] == 0) {
                dfs(n);
            }
        }
        visited_[cur_node] = 2;
    }

    void CheckCycles() {
        visited_.clear();
        for (auto& task_ptr : tasks_) {
            if (visited_[task_ptr.get()] == 0) {
                dfs(task_ptr.get());
            }
        }
    }
};

#include "TTaskScheduler.tpp"