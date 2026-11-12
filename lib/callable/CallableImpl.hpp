#pragma once

#include <memory>
#include <type_traits>

#include "ICallable.hpp"

template <typename Func>
class CallableImpl : public ICallable {
private:
    using ReturnT = std::invoke_result_t<Func>;
    using DecayT  = std::decay_t<ReturnT>;

    Func func_;
    std::shared_ptr<DecayT>* result_slot_; 

public:
    CallableImpl(Func&& f, std::shared_ptr<DecayT>& res)
        : func_(std::move(f)), result_slot_(&res) {}

    void Run() override {
        *result_slot_ = std::make_shared<DecayT>(func_());
    }
};