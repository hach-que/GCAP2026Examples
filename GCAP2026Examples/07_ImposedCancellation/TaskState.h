#pragma once

#include <coroutine>
#include <functional>
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

// (forward declaration)
template <typename ReturnType>
struct TTaskStateBase;

// The function signature for continuations that need to run after a task has a result.
template <typename ReturnType>
using TTaskStateContinuation = std::function<void(bool bIsCancellation, TTaskStateBase<ReturnType> &)>;

template <typename ReturnType>
struct TTaskStateBase : public std::enable_shared_from_this<TTaskStateBase<ReturnType>>
{
protected:
    // The result of task.
    TTaskStateValueVariant<ReturnType> Value;

    // A list of callbacks to fire when the value is ready.
    std::vector<TTaskStateContinuation<ReturnType>> Continuations;

public:
    // Sets up the first callback always set the Value field.
    TTaskStateBase()
        : Value()
        , Continuations()
    {
    }

    virtual bool IsValid() const = 0;

    bool IsReadyToResume() const
    {
        return this->Value.index() != VariantIndex_Empty && this->Value.index() != VariantIndex_Cancelled;
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
        // @note: Keep the TTaskState alive while we're running continuations, since
        // executing continuations may result in all other references to the task state
        // being released.
        std::shared_ptr<TTaskStateBase<ReturnType>> Pin = this->shared_from_this();

        bool bIsCancellation = this->Value.index() == VariantIndex_Cancelled;
        for (const auto &Continuation : this->Continuations)
        {
            Continuation(bIsCancellation, *Pin);
        }
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
        auto StatePtr = SuspendingParentCoroutine.promise().get_internal_task_state();

        // If we do not have a value yet, then our task hasn't returned and we need to resume the suspending parent
        // coroutine when we get a result.
        if (this->Value.index() == VariantIndex_Empty)
        {
            // Continue the parent coroutine when this task is done.
            this->Continuations.push_back([SuspendingParentCoroutine,
                                           // @note: Does this hold the lifetime of StatePtr too long? It shouldn't.
                                           StatePtr](bool bIsCancellation, const auto &) {
                if (!bIsCancellation)
                {
                    SuspendingParentCoroutine.promise().promise_continue(
                        [SuspendingParentCoroutine]() {
                            SuspendingParentCoroutine.resume();
                        },
                        [SuspendingParentCoroutine]() {
                            SuspendingParentCoroutine.destroy();
                        });
                }
                else
                {
                    SuspendingParentCoroutine.promise().promise_cancel([SuspendingParentCoroutine]() {
                        SuspendingParentCoroutine.destroy();
                    });
                }
            });
        }
        else
        {
            // We already have a value set, so we need to run the continuation immediately.

            // Check if our result was a cancellation.
            bool bIsCancellation = this->Value.index() == VariantIndex_Cancelled;

            // Run the continuation immediately.
            if (!bIsCancellation)
            {
                SuspendingParentCoroutine.promise().promise_continue(
                    [SuspendingParentCoroutine]() {
                        SuspendingParentCoroutine.resume();
                    },
                    [SuspendingParentCoroutine]() {
                        SuspendingParentCoroutine.destroy();
                    });
            }
            else
            {
                SuspendingParentCoroutine.promise().promise_cancel([SuspendingParentCoroutine]() {
                    SuspendingParentCoroutine.destroy();
                });
            }
        }
    }
};

// @note: We no longer need a specialization of TTaskStateBase<void> because we specialize with FVoidCompleteIfNeeded
// and requires() instead.

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