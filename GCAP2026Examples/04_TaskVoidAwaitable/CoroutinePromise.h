#pragma once

#include "TaskState.h"
#include <coroutine>
#include <memory>

template <typename ReturnType>
struct TCoroutinePromise
{
    std::shared_ptr<TTaskState<ReturnType>> State;

    TCoroutinePromise()
        : State(std::make_shared<TTaskState<ReturnType>>())
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
};

// Specialization for void.
template <>
struct TCoroutinePromise<void>
{
    std::shared_ptr<TTaskState<void>> State;

    TCoroutinePromise()
        : State(std::make_shared<TTaskState<void>>())
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
};