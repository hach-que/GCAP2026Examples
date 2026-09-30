#pragma once

#include "CoroutinePromisePtr.h"
#include "DebugPrintf.h"
#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <cstdio>
#include <memory>

template <typename ReturnType>
struct TCoroutinePromiseBase
{
    friend TTask<ReturnType, ETaskBinding::Unspecified>;
    friend TTask<ReturnType, ETaskBinding::Static>;
    friend TTask<ReturnType, ETaskBinding::Unbound>;
    friend TCoroutinePromisePtr<ReturnType>;
    friend TTaskState<ReturnType>;

public:
    TTaskState<ReturnType> State;

private:
    int RefCount;
    bool HasRunningRef;

    void IncreaseRef() noexcept
    {
        this->RefCount++;
        DEBUG_PRINTF("%p ref count is now %d\n", this, this->RefCount);
    }

    void DecreaseRef() noexcept
    {
        auto Count = --this->RefCount;
        DEBUG_PRINTF("%p ref count is now %d\n", this, Count);
        if (Count == 0)
        {
            DEBUG_PRINTF("%p is being destroyed\n", this);
            std::coroutine_handle<TCoroutinePromiseBase<ReturnType>>::from_promise(*this).destroy();
        }
    }

    void RemoveRunningRef() noexcept
    {
        if (this->HasRunningRef)
        {
            this->HasRunningRef = false;
            this->DecreaseRef();
        }
    }

public:
    TCoroutinePromiseBase(FTaskLifetimeBinding &&InLifetime)
        : State(std::move(InLifetime))
        // coroutine itself (final_suspend) plus initial TTask that will reference this coroutine
        , RefCount(2)
        , HasRunningRef(true)
    {
    }

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    struct FCoroutinePromiseFinal
    {
        TCoroutinePromiseBase *Ptr;
        bool await_ready() const noexcept
        {
            // Allow immediate destruction if only the coroutine is holding a reference.
            DEBUG_PRINTF("%p has ref count %d in FCoroutinePromiseFinal\n", Ptr, Ptr->RefCount);
            return Ptr->RefCount == 1 && Ptr->HasRunningRef;
        }
        void await_suspend(auto) noexcept
        {
            // Allow a TTask to ref count this to 0 for destruction outside of coroutine machinary.
            DEBUG_PRINTF("%p is letting a TTask destroy it later\n", Ptr);
            Ptr->RemoveRunningRef();
        }
        void await_resume() noexcept
        {
        }
    };

    FCoroutinePromiseFinal final_suspend() noexcept
    {
        DEBUG_PRINTF("%p reached final_suspend\n", this);
        return FCoroutinePromiseFinal{this};
    }

    void unhandled_exception()
    {
    }

    void TryResume(bool bIsCancellation)
    {
        // We are no longer awaiting on anything.
        this->State.AwaitingOn = nullptr;

        if (bIsCancellation || this->State.bIsCancellationRequested)
        {
            // We are cancelled by the thing we awaited, or because cancellation was requested on us since we started
            // awaiting.
            this->State.EmplaceCancellation();
            DEBUG_PRINTF("coroutine is cancelling; releasing ref\n");
            this->RemoveRunningRef();
            return;
        }

        if (!IsBound(this->State.Lifetime))
        {
            // The task we were waiting on returned successfully (or threw an exception), but we ourselves are no longer
            // bound and thus need to cancel. We won't be resuming, so destroy ourselves.
            this->State.EmplaceCancellation();
            DEBUG_PRINTF("coroutine is cancelling; releasing ref\n");
            this->RemoveRunningRef();
            return;
        }

        // Otherwise we're ready to resume.
        std::coroutine_handle<TCoroutinePromiseBase<ReturnType>>::from_promise(*this).resume();
    }
};

template <typename ReturnType, ETaskBinding TaskBinding>
struct TCoroutinePromise : public TCoroutinePromiseBase<ReturnType>
{
    TCoroutinePromise(FTaskLifetimeBinding &&InLifetime)
        : TCoroutinePromiseBase<ReturnType>(std::move(InLifetime))
    {
    }

    TTask<ReturnType, TaskBinding> get_return_object()
    {
        return TTask<ReturnType, TaskBinding>(TCoroutinePromisePtr<ReturnType>(this));
    }

    void return_value(ReturnType &&Value)
    {
        this->State.EmplaceValue(Value);
    }

    void return_value(const ReturnType &Value)
    {
        this->State.EmplaceValue(Value);
    }
};

// Specialization for void.
template <ETaskBinding TaskBinding>
struct TCoroutinePromise<void, TaskBinding> : public TCoroutinePromiseBase<void>
{
    TCoroutinePromise(FTaskLifetimeBinding &&InLifetime)
        : TCoroutinePromiseBase<void>(std::move(InLifetime))
    {
    }

    TTask<void, TaskBinding> get_return_object()
    {
        return TTask<void, TaskBinding>(TCoroutinePromisePtr<void>(this));
    }

    void return_void()
    {
        this->State.EmplaceValue();
    }
};