#pragma once

#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType, ETaskBinding TaskBinding>
struct TTask
{
    friend TCoroutinePromiseBase<ReturnType, TaskBinding>;
    friend TTask<ReturnType, ETaskBinding::Unspecified>;
    friend TTask<ReturnType, ETaskBinding::Static>;
    friend TTask<ReturnType, ETaskBinding::Unbound>;

protected:
    std::shared_ptr<TTaskStateBase<ReturnType>> State;

    explicit TTask(std::shared_ptr<TTaskStateBase<ReturnType>> InState)
        : State(InState)
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
        return TTask<ReturnType>(this->State);
    }

    bool await_ready() const
    {
        return this->State->IsReadyToResume();
    }

    // @note: In an actual task library you'd probably want to hide this from the public API to prevent users from
    // depending on internals. For this example however, we're just exposing it directly.
    auto get_internal_task_state() const
    {
        return this->State;
    }

    template <typename SuspendingParentCoroutineType>
    void await_suspend(std::coroutine_handle<SuspendingParentCoroutineType> Handle)
    {
        this->State->AddContinuationFromSuspendingParentCoroutine(Handle);
    }

    // --- REQUESTED CANCELLATION SUPPORT

    void RequestCancellation()
    {
        this->State->RequestCancellation();
    }

    // --- END REQUESTED CANCELLATION SUPPORT

    // For non-void return values:

    ReturnType await_resume()
        requires(!std::is_void_v<ReturnType>)
    {
        return this->State->GetValue();
    }

    // For void return values:

    void await_resume()
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
        : TTask<ReturnType, ETaskBinding::Unspecified>(std::make_shared<TTaskStateStatic<ReturnType>>())
    {
    }

public:
    void SetValue(typename FVoidCompleteIfNeeded<ReturnType>::Type &&Result)
        requires(!std::is_void_v<ReturnType>)
    {
        this->State->EmplaceValue(Result);
    }

    void SetValue(const typename FVoidCompleteIfNeeded<ReturnType>::Type &Result)
        requires(!std::is_void_v<ReturnType>)
    {
        this->State->EmplaceValue(Result);
    }

    void SetValue()
        requires(std::is_void_v<ReturnType>)
    {
        this->State->EmplaceValue();
    }

    void SetCancelled()
    {
        this->State->EmplaceCancellation();
    }

    // --- REQUESTED CANCELLATION SUPPORT

    std::function<void()> OnCancellationRequested(const std::function<void()> &Callback) const
    {
        return this->State->AddCustomCancellationHandler([WeakPtr = this->State->weak_from_this(), Callback]() {
            auto PinnedPtr = WeakPtr.lock();
            if (PinnedPtr != nullptr)
            {
                if (PinnedPtr->IsValid() && !PinnedPtr->IsReadyToResume() && !PinnedPtr->IsCancelled())
                {
                    Callback();
                }
            }
        });
    }

    // --- END REQUESTED CANCELLATION SUPPORT
};

template <typename ReturnType, ETaskBinding TaskBinding>
TDeferredTask<ReturnType> TTask<ReturnType, TaskBinding>::Deferred()
{
    return TDeferredTask<ReturnType>();
}