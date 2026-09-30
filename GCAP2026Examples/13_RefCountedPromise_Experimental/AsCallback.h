#pragma once

#include "ForwardDecl.h"
#include "Task.h"
#include "TaskState.h"
#include <functional>

template <typename ReturnType>
struct FAsCallbackTypes
{
    using OnCompletion = std::function<void(ReturnType)>;
};

template <>
struct FAsCallbackTypes<void>
{
    using OnCompletion = std::function<void()>;
};

template <typename ReturnType, ETaskBinding TaskBinding>
void AsCallback(
    TTask<ReturnType, TaskBinding> Task,
    typename FAsCallbackTypes<ReturnType>::OnCompletion OnCompletion,
    std::function<void()> OnCancellation)
{
    Task.GetState().AddContinuation(
        // @note: Task capture ensures that TaskState remains alive at least until this continuation runs.
        [Task, OnCompletion, OnCancellation](bool bIsCancellation, TTaskState<ReturnType> &State) {
            if (bIsCancellation)
            {
                OnCancellation();
            }
            else
            {
                if constexpr (std::is_void_v<ReturnType>)
                {
                    OnCompletion();
                }
                else
                {
                    OnCompletion(State.GetValue());
                }
            }
        });
}