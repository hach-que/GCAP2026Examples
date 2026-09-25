#pragma once

#include "ForwardDecl.h"
#include "TaskBinding.h"
#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType, ETaskBinding TaskBinding>
struct TTask
{
    friend struct TCoroutinePromiseBase<ReturnType, TaskBinding>;
    friend struct TTask<ReturnType, ETaskBinding::Unspecified>;
    friend struct TTask<ReturnType, ETaskBinding::Static>;
    friend struct TTask<ReturnType, ETaskBinding::Unbound>;

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