#pragma once

#include <functional>
#include <memory>
#include <optional>

template <typename ReturnType>
struct TTaskStateBase
{
    // The returned value.
    std::optional<ReturnType> Value;

    // A list of callbacks to fire when the value is ready.
    std::vector<std::function<void(ReturnType)>> Callbacks;

    // Sets up the first callback always set the Value field.
    TTaskStateBase()
        : Value()
        , Callbacks(std::vector<std::function<void(ReturnType)>>{[this](ReturnType InValue) {
            this->Value = InValue;
        }})
    {
    }

    virtual bool IsValid() const = 0;
};

template <>
struct TTaskStateBase<void>
{
    // True if the coroutine has completed.
    bool bComplete;

    // A list of callbacks to fire when the value is ready.
    std::vector<std::function<void()>> Callbacks;

    // Sets up the first callback always set the Value field.
    TTaskStateBase()
        : bComplete()
        , Callbacks(std::vector<std::function<void()>>{[this]() {
            this->bComplete = true;
        }})
    {
    }

    virtual bool IsValid() const = 0;
};

// For static functions
template <typename ReturnType>
struct TTaskStateStatic : public TTaskStateBase<ReturnType>
{
    TTaskStateStatic()
        : TTaskStateBase<ReturnType>()
    {
    }

    virtual bool IsValid() const
    {
        return true;
    }
};

// For functions declared on something that inherits std::enable_shared_from_this
template <typename ReturnType>
struct TTaskStateSharedFromThis : public TTaskStateBase<ReturnType>
{
    std::weak_ptr<void> Owner;

    TTaskStateSharedFromThis(std::weak_ptr<void> InOwner)
        : TTaskStateBase<ReturnType>()
        , Owner(InOwner)
    {
    }

    virtual bool IsValid() const
    {
        return !this->Owner.expired();
    }
};