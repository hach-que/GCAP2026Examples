#pragma once

#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType>
struct TCoroutinePromise
{
    std::shared_ptr<TTaskStateBase<ReturnType>> State;

    TCoroutinePromise(std::shared_ptr<TTaskStateBase<ReturnType>> InState)
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

    void return_value(ReturnType Value)
    {
        auto Callbacks = this->State->Callbacks;
        this->State->Callbacks.clear();
        for (const auto &Cb : Callbacks)
        {
            Cb(Value);
        }
    }

    TTask<ReturnType> get_return_object()
    {
        return TTask<ReturnType>(this->State);
    }

    // -------- Add promise validity passthrough --------

    bool promise_is_valid() const
    {
        return this->State->IsValid();
    }
};

// Specialization for void.
template <>
struct TCoroutinePromise<void>
{
    std::shared_ptr<TTaskStateBase<void>> State;

    TCoroutinePromise(std::shared_ptr<TTaskStateBase<void>> InState)
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

    void return_void()
    {
        auto Callbacks = this->State->Callbacks;
        this->State->Callbacks.clear();
        for (const auto &Cb : Callbacks)
        {
            Cb();
        }
    }

    TTask<void> get_return_object()
    {
        return TTask<void>(this->State);
    }

    bool promise_is_valid() const
    {
        return this->State->IsValid();
    }
};