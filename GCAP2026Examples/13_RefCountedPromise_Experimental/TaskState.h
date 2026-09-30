#pragma once

#include "TaskLifetimeBinding.h"
#include <coroutine>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <variant>

// Indicates that the task does not yet have a result.
struct FEmptyVariantState
{
};

// Indicates that the task was cancelled.
struct FCancelledTaskVariantState
{
};

// Used to represent a void task completed (since it has no value).
struct FVoidComplete
{
};

// This template maps void to FVoidComplete, otherwise passes through the type as-is.
template <typename ReturnType>
struct FVoidCompleteIfNeeded
{
    using Type = ReturnType;
};
template <>
struct FVoidCompleteIfNeeded<void>
{
    using Type = FVoidComplete;
};

// The variant type that contains the possible results inside a task state.
template <typename ReturnType>
using TTaskStateValueVariant =
    std::variant<FEmptyVariantState, FCancelledTaskVariantState, typename FVoidCompleteIfNeeded<ReturnType>::Type>;
constexpr std::size_t VariantIndex_Empty = 0;
constexpr std::size_t VariantIndex_Cancelled = 1;
constexpr std::size_t VariantIndex_Return = 2;

// The function signature for continuations that need to run after a task has a result.
template <typename ReturnType>
using TTaskStateContinuation = std::function<void(bool bIsCancellation, TTaskState<ReturnType> &)>;

class FTaskStateCancellationBase
{
public:
    FTaskStateCancellationBase *AwaitingOn;
    std::map<int, std::function<void()>> CustomCancellationBroadcast;
    int NextCustomCancellationBroadcastId;
    bool bIsCancellationRequested;

    FTaskStateCancellationBase()
        : AwaitingOn(nullptr)
        , CustomCancellationBroadcast()
        , NextCustomCancellationBroadcastId()
        , bIsCancellationRequested(false)
    {
    }
};

template <typename ReturnType>
struct TTaskState : public FTaskStateCancellationBase
{
protected:
    // The result of task.
    TTaskStateValueVariant<ReturnType> Value;

    // A list of callbacks to fire when the value is ready.
    std::vector<TTaskStateContinuation<ReturnType>> Continuations;

public:
    // The lifetime of this task.
    FTaskLifetimeBinding Lifetime;

public:
    TTaskState(FTaskLifetimeBinding &&InLifetime)
        : FTaskStateCancellationBase()
        , Value()
        , Continuations()
        , Lifetime(InLifetime)
    {
    }

    bool IsReadyToResume() const
    {
        return this->Value.index() != VariantIndex_Empty && this->Value.index() != VariantIndex_Cancelled;
    }

    void RequestCancellation()
    {
        if (this->bIsCancellationRequested)
        {
            return;
        }

        std::vector<std::function<void()>> Broadcasts;
        this->bIsCancellationRequested = true;
        for (const auto &KV : this->CustomCancellationBroadcast)
        {
            Broadcasts.push_back(KV.second);
        }
        while (this->AwaitingOn != nullptr)
        {
            if (this->AwaitingOn->bIsCancellationRequested)
            {
                break;
            }

            this->AwaitingOn->bIsCancellationRequested = true;
            for (const auto &KV : this->AwaitingOn->CustomCancellationBroadcast)
            {
                Broadcasts.push_back(KV.second);
            }
            this->AwaitingOn = this->AwaitingOn->AwaitingOn;
        }

        for (const auto &Broadcast : Broadcasts)
        {
            Broadcast();
        }
    }

    bool IsCancellationRequested() const
    {
        return this->bIsCancellationRequested;
    }

    bool IsCancelled() const
    {
        return this->Value.index() == VariantIndex_Cancelled;
    }

    const typename FVoidCompleteIfNeeded<ReturnType>::Type &GetValue() const
        requires(!std::is_void_v<ReturnType>)
    {
        return std::get<VariantIndex_Return>(this->Value);
    }

private:
    void Continue()
    {
        this->AwaitingOn = nullptr;
        this->CustomCancellationBroadcast.clear();

        bool bIsCancellation = this->Value.index() == VariantIndex_Cancelled;
        for (const auto &Continuation : this->Continuations)
        {
            Continuation(bIsCancellation, *this);
        }

        // @note: Must clear, since continuations may hold references to tasks.
        this->Continuations.clear();
    }

public:
    void EmplaceValue(typename FVoidCompleteIfNeeded<ReturnType>::Type &&Result)
        requires(!std::is_void_v<ReturnType>)
    {
        this->Value.template emplace<ReturnType>(std::move(Result));
        this->Continue();
    }

    void EmplaceValue(const typename FVoidCompleteIfNeeded<ReturnType>::Type &Result)
        requires(!std::is_void_v<ReturnType>)
    {
        this->Value.template emplace<ReturnType>(Result);
        this->Continue();
    }

    void EmplaceValue()
        requires std::is_void_v<ReturnType>
    {
        this->Value.template emplace<FVoidComplete>();
        this->Continue();
    }

    void EmplaceCancellation()
    {
        this->Value.template emplace<FCancelledTaskVariantState>();
        this->Continue();
    }

    void AddContinuation(TTaskStateContinuation<ReturnType> &&Continuation)
    {
        if (this->Value.index() == VariantIndex_Empty)
        {
            this->Continuations.push_back(std::move(Continuation));
        }
        else
        {
            bool bIsCancellation = this->Value.index() == VariantIndex_Cancelled;
            Continuation(bIsCancellation, *this);
        }
    }

    template <typename SuspendingParentCoroutineType>
    void AddContinuationFromSuspendingParentCoroutine(
        std::coroutine_handle<SuspendingParentCoroutineType> SuspendingParentCoroutine)
    {
        // Make sure the parent coroutine has a state.
        auto &ParentState = SuspendingParentCoroutine.promise().State;

        // Check if the parent coroutine is already cancelling.
        bool bIsParentAlreadyCancelling = ParentState.IsCancellationRequested();

        // If we do not have a value yet, then our task hasn't returned and we need to resume the suspending parent
        // coroutine when we get a result.
        if (this->Value.index() == VariantIndex_Empty)
        {
            // If the parent coroutine is not already cancelling, tell it what it is awaiting so it can send
            // cancellation requests to us.
            if (!bIsParentAlreadyCancelling)
            {
                ParentState.AwaitingOn = this;
            }

            // Continue the parent coroutine when this task is done.
            this->Continuations.push_back([SuspendingParentCoroutine](bool bIsCancellation, const auto &) {
                SuspendingParentCoroutine.promise().TryResume(bIsCancellation);
            });

            // Now immediately cancel our coroutine if the parent has requested cancellation, which will cause our work
            // to stop and will propagate cancellation back to the parent that is yet to act on the requested
            // cancellation.
            if (bIsParentAlreadyCancelling)
            {
                this->RequestCancellation();
            }
        }
        else
        {
            // We already have a value set, so we need to run the continuation immediately.

            // Check if our result was a cancellation.
            bool bIsCancellation = this->Value.index() == VariantIndex_Cancelled;

            // Run the continuation immediately.
            SuspendingParentCoroutine.promise().TryResume(bIsCancellation);
        }
    }

    std::function<void()> AddCustomCancellationHandler(const std::function<void()> &InOnCancellationRequested)
    {
        // @note: Does not fire cancellation immediately because AddContinuation may not yet have been called.
        if (this->Value.index() == VariantIndex_Empty)
        {
            int BroadcastId = this->NextCustomCancellationBroadcastId++;
            this->CustomCancellationBroadcast.emplace(BroadcastId, InOnCancellationRequested);
            return [this, BroadcastId]() {
                // @note: Assumes you don't try to deregister a custom cancellation broadcast after the task state has
                // gone away.
                this->CustomCancellationBroadcast.erase(BroadcastId);
            };
        }
        else
        {
            return []() {
            };
        }
    }
};

template <typename ReturnType>
using TTaskStateRef = std::variant<TCoroutinePromisePtr<ReturnType>, std::shared_ptr<TTaskState<ReturnType>>>;