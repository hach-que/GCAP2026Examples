#pragma once

#include "CoroutinePromise.h"
#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>
#include <variant>

template <typename ReturnType, ETaskBinding TaskBinding>
struct TTask
{
    friend TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>;
    friend TCoroutinePromise<ReturnType, ETaskBinding::Static>;
    friend TCoroutinePromise<ReturnType, ETaskBinding::Unbound>;
    friend TTask<ReturnType, ETaskBinding::Unspecified>;
    friend TTask<ReturnType, ETaskBinding::Static>;
    friend TTask<ReturnType, ETaskBinding::Unbound>;

protected:
    TTaskStateRef<ReturnType> StateRef;

    explicit TTask(TTaskStateRef<ReturnType> InStateRef)
        : StateRef(InStateRef)
    {
    }

    TTask() = default;

public:
    TTask(const TTask &) = default;
    TTask(TTask &&) = default;
    ~TTask() = default;

    TTask &operator=(const TTask &) = default;
    TTask &operator=(TTask &&) = default;

    static TDeferredTask<ReturnType> Deferred();

    // For casting away ETaskBinding on the template:

    operator TTask<ReturnType>()
    {
        return TTask<ReturnType>(this->StateRef);
    }

    bool await_ready() const
    {
        return this->GetState().IsReadyToResume();
    }

    template <typename SuspendingParentCoroutineType>
    void await_suspend(std::coroutine_handle<SuspendingParentCoroutineType> Handle) const
    {
        this->GetState().AddContinuationFromSuspendingParentCoroutine(Handle);
    }

    void RequestCancellation() const
    {
        this->GetState().RequestCancellation();
    }

    // @note: Hide this from the public API.
    TTaskState<ReturnType> &GetState() const
    {
        if (this->StateRef.index() == 0)
        {
            return std::get<0>(this->StateRef)->State;
        }

        return *std::get<1>(this->StateRef).get();
    }

    // For non-void return values:

    ReturnType await_resume() const
        requires(!std::is_void_v<ReturnType>)
    {
        return this->GetState().GetValue();
    }

    // For void return values:

    void await_resume() const
        requires(std::is_void_v<ReturnType>)
    {
    }
};

template <typename ReturnType>
struct TDeferredTask : public TTask<ReturnType, ETaskBinding::Unspecified>
{
    friend TTask<ReturnType, ETaskBinding::Unspecified>;

private:
    explicit TDeferredTask()
        : TTask<ReturnType, ETaskBinding::Unspecified>(
              std::make_shared<TTaskState<ReturnType>>(FTaskLifetimeBindingStatic()))
    {
    }

public:
    void SetValue(typename FVoidCompleteIfNeeded<ReturnType>::Type &&Result) const
        requires(!std::is_void_v<ReturnType>)
    {
        this->GetState().EmplaceValue(Result);
    }

    void SetValue(const typename FVoidCompleteIfNeeded<ReturnType>::Type &Result) const
        requires(!std::is_void_v<ReturnType>)
    {
        this->GetState().EmplaceValue(Result);
    }

    void SetValue() const
        requires(std::is_void_v<ReturnType>)
    {
        this->GetState().EmplaceValue();
    }

    void SetCancelled() const
    {
        this->GetState().EmplaceCancellation();
    }

    std::function<void()> OnCancellationRequested(const std::function<void()> &Callback) const
    {
        return this->GetState().AddCustomCancellationHandler([Task = *this, Callback]() {
            if (IsBound(Task.GetState().Lifetime) && !Task.GetState().IsReadyToResume() &&
                !Task.GetState().IsCancelled())
            {
                Callback();
            }
        });
    }
};

template <typename ReturnType, ETaskBinding TaskBinding>
TDeferredTask<ReturnType> TTask<ReturnType, TaskBinding>::Deferred()
{
    return TDeferredTask<ReturnType>();
}