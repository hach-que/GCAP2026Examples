#pragma once

#include "AsCallback.h"
#include "ForwardDecl.h"
#include <optional>

enum class EVoidPotentiallyCancelled : uint8_t
{
    Returned,
    Cancelled,
};

template <typename ReturnType>
struct FAsPotentiallyCancelledTypes
{
    using TaskType = TTask<std::optional<ReturnType>>;
};

template <>
struct FAsPotentiallyCancelledTypes<void>
{
    using TaskType = TTask<EVoidPotentiallyCancelled>;
};

template <typename ReturnType, ETaskBinding TaskBinding>
FAsPotentiallyCancelledTypes<ReturnType>::TaskType AsPotentiallyCancelled(TTask<ReturnType, TaskBinding> Task)
{
    auto Deferred = FAsPotentiallyCancelledTypes<ReturnType>::TaskType::Deferred();
    Deferred.OnCancellationRequested([Task]() mutable {
        // Propagate requested cancellation downwards.
        Task.RequestCancellation();
    });
    if constexpr (!std::is_void_v<ReturnType>)
    {
        AsCallback(
            Task,
            [Deferred](const auto &Value) mutable {
                Deferred.SetValue(std::optional<ReturnType>(Value));
            },
            [Deferred]() mutable {
                Deferred.SetValue(std::optional<ReturnType>());
            });
    }
    else
    {
        AsCallback(
            Task,
            [Deferred]() mutable {
                Deferred.SetValue(EVoidPotentiallyCancelled::Returned);
            },
            [Deferred]() mutable {
                Deferred.SetValue(EVoidPotentiallyCancelled::Cancelled);
            });
    }
    return Deferred;
}