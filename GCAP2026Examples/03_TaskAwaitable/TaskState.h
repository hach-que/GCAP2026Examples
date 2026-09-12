#pragma once

#include <functional>
#include <optional>

// Our shared state that we're going to reference from our return type.
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