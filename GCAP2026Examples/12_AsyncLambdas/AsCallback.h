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
    Task.get_internal_task_state()->AddContinuation(
        [OnCompletion, OnCancellation](bool bIsCancellation, TTaskStateBase<ReturnType> &State) {
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