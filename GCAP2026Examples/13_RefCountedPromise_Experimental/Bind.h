#pragma once

#include "AsCallback.h"
#include "IUnbound.h"
#include "Task.h"

// --- BIND PROVIDERS ---

// Base struct for bind providers, which includes the concrete implementation of IUnbound for Bind() to use.
struct BindProviderBase
{
protected:
    class __DO_NOT_USE_EXTERNALLY_Bound : public IUnbound
    {
    public:
        explicit __DO_NOT_USE_EXTERNALLY_Bound()
            : IUnbound() {};

        __DO_NOT_USE_EXTERNALLY_Bound(const __DO_NOT_USE_EXTERNALLY_Bound &) = default;
        __DO_NOT_USE_EXTERNALLY_Bound(__DO_NOT_USE_EXTERNALLY_Bound &&) = default;
        ~__DO_NOT_USE_EXTERNALLY_Bound() = default;
    };
};

// The anonymous classes generated for lambdas overload the 'operator()'. We want our bind provider to specialize on the
// function signature of 'operator()' so that we match on return types and parameters, and not the anonymous lambda
// class itself.
template <typename T>
struct BindProvider : public BindProvider<decltype(&std::remove_reference_t<T>::operator())>
{
};

// Concrete bind provider for lambdas.
template <typename ReturnType, typename... Params>
struct BindProvider<TTask<ReturnType, ETaskBinding::Unbound>(IUnbound, Params...)> : private BindProviderBase
{
private:
    // Wrapper on AsCallback to simplify BindHeap implementations below.
    template <typename LambdaType>
    static void RouteViaAsCallback(
        const TDeferredTask<ReturnType> &Deferred,
        const TTask<ReturnType> &Task,
        const std::shared_ptr<LambdaType> &LambdaOnHeap)
    {
        // @note: Our AsCallback calls capture the 'LambdaOnHeap' variable (even though it is never used), because this
        // ensures the lambda on the heap is not freed until the coroutine has finished or been cancelled.
        if constexpr (std::is_void_v<ReturnType>)
        {
            AsCallback(
                Task,
                [LambdaOnHeap, Deferred]() {
                    Deferred.SetValue();
                },
                [LambdaOnHeap, Deferred] {
                    Deferred.SetCancelled();
                });
        }
        else
        {
            AsCallback(
                Task,
                [LambdaOnHeap, Deferred](const ReturnType &Result) {
                    Deferred.SetValue(Result);
                },
                [LambdaOnHeap, Deferred] {
                    Deferred.SetCancelled();
                });
        }
    }

    // Bind an async lambda that is already on the heap to a global (static) lifetime.
    template <typename LambdaType>
    static auto BindHeap(const std::shared_ptr<LambdaType> &LambdaOnHeap)
    {
        return [LambdaOnHeap](Params... Args) -> TTask<ReturnType> {
            auto Deferred = TTask<ReturnType>::Deferred();
            TTask<ReturnType, ETaskBinding::Unbound> Task = ([&] {
                if constexpr (requires {
                                  (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, std::forward<Params...>(Args)...);
                              })
                {
                    return (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, std::forward<Params...>(Args)...);
                }
                else
                {
                    return (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, Args...);
                }
            }());
            RouteViaAsCallback(Deferred, Task, LambdaOnHeap);
            return Deferred;
        };
    }

    // Bind an async lambda that is already on the heap to an std::weak_ptr's lifetime.
    template <typename ClassType, typename LambdaType>
    static auto BindHeap(
        const std::weak_ptr<const ClassType> &WeakObjectPtr,
        const std::shared_ptr<LambdaType> &LambdaOnHeap)
    {
        return [LambdaOnHeap, WeakObjectPtr](Params... Args) -> TTask<ReturnType> {
            auto Deferred = TTask<ReturnType>::Deferred();
            TTask<ReturnType, ETaskBinding::Unbound> Task = ([&] {
                if constexpr (requires {
                                  (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, std::forward<Params...>(Args)...);
                              })
                {
                    return (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, std::forward<Params...>(Args)...);
                }
                else
                {
                    return (*LambdaOnHeap)(__DO_NOT_USE_EXTERNALLY_Bound{}, Args...);
                }
            }());

            // @note: This is the point where we associate the task with the desired lifetime.
            TryBindToSharedPtr(Task.GetState().Lifetime, WeakObjectPtr);
            if (!IsBound(Task.GetState().Lifetime))
            {
                // Never return; object is no longer valid.
                Deferred.SetCancelled();
                return Deferred;
            }

            RouteViaAsCallback(Deferred, Task, LambdaOnHeap);
            return Deferred;
        };
    }

public:
    // Move the lambda to the heap, then bind it to the static (global) lifetime.
    template <typename LambdaType>
    static auto Bind(LambdaType &&Lambda)
    {
        return BindHeap(std::make_shared<std::remove_reference_t<LambdaType>>(std::forward<LambdaType>(Lambda)));
    }

    // Move the lambda to the heap, then bind it to an std::weak_ptr's lifetime.
    template <typename ClassType, typename LambdaType>
    static auto Bind(const std::weak_ptr<const ClassType> &WeakObjectPtr, LambdaType &&Lambda)
    {
        return BindHeap(
            WeakObjectPtr,
            std::make_shared<std::remove_reference_t<LambdaType>>(std::forward<LambdaType>(Lambda)));
    }
};

// Bind providers to match on slightly different function signatures and route them all to our common bind provider
// implementation.
template <typename ReturnType, typename... Args>
struct BindProvider<ReturnType (*)(Args...)> : public BindProvider<ReturnType(Args...)>
{
};
template <typename ClassType, typename ReturnType, typename... Args>
struct BindProvider<ReturnType (ClassType::*)(Args...)> : public BindProvider<ReturnType(Args...)>
{
};
template <typename ClassType, typename ReturnType, typename... Args>
struct BindProvider<ReturnType (ClassType::*)(Args...) const> : public BindProvider<ReturnType(Args...)>
{
};

// --- BIND FUNCTIONS ---

// Bind an asynchronous lambda to a global (static) lifetime.
template <typename LambdaType>
[[nodiscard]] auto Bind(LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<LambdaType>(std::forward<LambdaType>(Lambda));
}

// Bind an asynchronous lambda to the lifetime of a std::shared_ptr.
template <typename ClassType, typename LambdaType>
[[nodiscard]] auto Bind(const std::shared_ptr<ClassType> &Object, LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<ClassType, LambdaType>(Object, std::forward<LambdaType>(Lambda));
}

// Bind an asynchronous lambda to the lifetime of a std::shared_ptr.
template <typename ClassType, typename LambdaType>
[[nodiscard]] auto Bind(const std::shared_ptr<const ClassType> &Object, LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<ClassType, LambdaType>(Object, std::forward<LambdaType>(Lambda));
}

// Bind an asynchronous lambda to the lifetime of a std::weak_ptr.
template <typename ClassType, typename LambdaType>
[[nodiscard]] auto Bind(const std::weak_ptr<ClassType> &Object, LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<ClassType, LambdaType>(Object, std::forward<LambdaType>(Lambda));
}

// Bind an asynchronous lambda to the lifetime of a std::weak_ptr.
template <typename ClassType, typename LambdaType>
[[nodiscard]] auto Bind(const std::weak_ptr<const ClassType> &Object, LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<ClassType, LambdaType>(Object, std::forward<LambdaType>(Lambda));
}

// Bind an asynchronous lambda to the lifetime of an object that implements std::enable_shared_from_this.
template <typename ClassType, typename LambdaType>
[[nodiscard]] auto Bind(const std::enable_shared_from_this<ClassType> *Object, LambdaType &&Lambda)
{
    return BindProvider<LambdaType>::template Bind<ClassType, LambdaType>(
        Object->shared_from_this(),
        std::forward<LambdaType>(Lambda));
}