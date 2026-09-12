#pragma once

#include "ForwardDecl.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

// Our return type that we're going to use for our coroutines, referencing the state shared between the coroutine
// "promise" and callers who want to await.
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

    // -------- TASK AWAITABLE START --------

    bool await_ready()
    {
        return this->State->Value.has_value();
    }

    void await_suspend(std::coroutine_handle<> Handle)
    {
        this->State->Callbacks.push_back([Handle](ReturnType) {
            Handle.resume();
        });
    }

    ReturnType await_resume()
    {
        return this->State->Value.value();
    }

    // -------- TASK AWAITABLE END --------
};