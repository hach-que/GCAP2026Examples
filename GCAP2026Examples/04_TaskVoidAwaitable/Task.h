#pragma once

#include "ForwardDecl.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType>
struct TTask
{
    friend struct TCoroutinePromise<ReturnType>;

private:
    std::shared_ptr<TTaskState<ReturnType>> State;

    explicit TTask(std::shared_ptr<TTaskState<ReturnType>> InState)
        : State(InState)
    {
    }

public:
    TTask() = default;
    TTask(const TTask &) = default;
    TTask(TTask &&) = default;
    ~TTask() = default;

    // For non-void return values:

    bool await_ready()
        requires(!std::is_void_v<ReturnType>)
    {
        return this->State->Value.has_value();
    }

    void await_suspend(std::coroutine_handle<> Handle)
        requires(!std::is_void_v<ReturnType>)
    {
        this->State->Callbacks.push_back([Handle](ReturnType) {
            Handle.resume();
        });
    }

    ReturnType await_resume()
        requires(!std::is_void_v<ReturnType>)
    {
        return this->State->Value.value();
    }

    // For void return values:

    bool await_ready()
        requires(std::is_void_v<ReturnType>)
    {
        return this->State->bComplete;
    }

    void await_suspend(std::coroutine_handle<> Handle)
        requires(std::is_void_v<ReturnType>)
    {
        this->State->Callbacks.push_back([Handle]() {
            Handle.resume();
        });
    }

    void await_resume()
        requires(std::is_void_v<ReturnType>)
    {
    }
};