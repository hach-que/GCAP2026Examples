#pragma once

#include <functional>
#include <optional>

template <typename ReturnType>
struct TTaskState
{
    // The returned value.
    std::optional<ReturnType> Value;

    // A list of callbacks to fire when the value is ready.
    std::vector<std::function<void(ReturnType)>> Callbacks;

    // Sets up the first callback always set the Value field.
    TTaskState()
        : Value()
        , Callbacks(std::vector<std::function<void(ReturnType)>>{[this](ReturnType InValue) {
            this->Value = InValue;
        }})
    {
    }
};

// Specialization for void.
template <>
struct TTaskState<void>
{
    // True if the coroutine has completed.
    bool bComplete;

    // A list of callbacks to fire when the value is ready.
    std::vector<std::function<void()>> Callbacks;

    // Sets up the first callback always set the Value field.
    TTaskState()
        : bComplete()
        , Callbacks(std::vector<std::function<void()>>{[this]() {
            this->bComplete = true;
        }})
    {
    }
};