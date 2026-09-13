#pragma once

#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TTask
{
    friend struct TCoroutinePromise<ReturnType, TaskBinding>;

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

    TTask &operator=(const TTask &) = default;
    TTask &operator=(TTask &&) = default;

    // For casting away ETaskBinding on the template:

    operator TTask<ReturnType>()
    {
        return TTask<ReturnType>(this->State);
    }

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