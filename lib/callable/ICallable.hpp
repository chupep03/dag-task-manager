#pragma once

class ICallable {
public:
    virtual void Run() = 0;
    ~ICallable() = default;
};