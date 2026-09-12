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
    std::shared_ptr<TTaskStateBase<ReturnType>> State;

    explicit TTask(std::shared_ptr<TTaskStateBase<ReturnType>> InState)
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

    template <typename CoroutinePromise>
    void await_suspend(std::coroutine_handle<CoroutinePromise> Handle)
        requires(!std::is_void_v<ReturnType>)
    {
        this->State->Callbacks.push_back([Handle](ReturnType) {
            // @note: see promise_is_valid on coroutine promise type to pass call down into task state
            if (Handle.promise().promise_is_valid())
            {
                Handle.resume();
            }
            else
            {
                // owner no longer valid
                std::terminate();
            }
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

    template <typename CoroutinePromise>
    void await_suspend(std::coroutine_handle<CoroutinePromise> Handle)
        requires(std::is_void_v<ReturnType>)
    {
        this->State->Callbacks.push_back([Handle]() {
            // @note: see promise_is_valid on coroutine promise type to pass call down into task state
            if (Handle.promise().promise_is_valid())
            {
                Handle.resume();
            }
            else
            {
                // owner no longer valid
                std::terminate();
            }
        });
    }

    void await_resume()
        requires(std::is_void_v<ReturnType>)
    {
    }
};