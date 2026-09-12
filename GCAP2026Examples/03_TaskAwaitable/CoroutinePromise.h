#pragma once

#include "TaskState.h"
#include <coroutine>
#include <memory>

// Our "promise type" that specifies how our coroutine will behave.
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