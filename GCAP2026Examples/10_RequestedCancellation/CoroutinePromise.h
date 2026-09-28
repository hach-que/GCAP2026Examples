#pragma once

#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType, ETaskBinding TaskBinding>
struct TCoroutinePromiseBase
{
protected:
    std::shared_ptr<TTaskStateBase<ReturnType>> State;

public:
    TCoroutinePromiseBase(std::shared_ptr<TTaskStateBase<ReturnType>> InState)
        : State(InState)
    {
    }

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_never final_suspend() noexcept
    {
        return {};
    }

    void unhandled_exception()
    {
    }

    TTask<ReturnType, TaskBinding> get_return_object()
    {
        return TTask<ReturnType, TaskBinding>(this->State);
    }

    void promise_continue(const std::function<void()> &Continuation, const std::function<void()> &Destroy)
    {
        if (!this->State)
        {
            Destroy();
            return;
        }

        if (!this->State->IsValid())
        {
            // The task we were waiting on returned successfully (or threw an exception), but we ourselves are no longer
            // bound and thus need to cancel. We won't be resuming, so destroy ourselves.
            this->State->EmplaceCancellation();
            Destroy();
            return;
        }

        Continuation();
    }

    void promise_cancel(const std::function<void()> &Destroy)
    {
        if (!this->State)
        {
            Destroy();
            return;
        }

        this->State->EmplaceCancellation();
        Destroy();
        return;
    }

    // @note: In an actual task library you'd probably want to hide this from the public API to prevent users from
    // depending on internals. For this example however, we're just exposing it directly.
    auto get_internal_task_state() const
    {
        return this->State;
    }
};

template <typename ReturnType, ETaskBinding TaskBinding>
struct TCoroutinePromise : public TCoroutinePromiseBase<ReturnType, TaskBinding>
{
    TCoroutinePromise(std::shared_ptr<TTaskStateBase<ReturnType>> InState)
        : TCoroutinePromiseBase<ReturnType, TaskBinding>(InState)
    {
    }

    void return_value(ReturnType &&Value)
    {
        this->State->EmplaceValue(Value);
    }

    void return_value(const ReturnType &Value)
    {
        this->State->EmplaceValue(Value);
    }
};

// Specialization for void.
template <ETaskBinding TaskBinding>
struct TCoroutinePromise<void, TaskBinding> : public TCoroutinePromiseBase<void, TaskBinding>
{
    TCoroutinePromise(std::shared_ptr<TTaskStateBase<void>> InState)
        : TCoroutinePromiseBase<void, TaskBinding>(InState)
    {
    }

    void return_void()
    {
        this->State->EmplaceValue();
    }
};