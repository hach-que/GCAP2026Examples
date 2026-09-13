#pragma once

#include "CoroutinePromise.h"
#include "Task.h"
#include "TaskBinding.h"
#include <memory>

template <typename... Args>
struct TCoroutineAsserts
{
    static_assert(
        (negation_v<is_reference<Args>> && ...),
        "Detected pass-by-reference for one or more arguments to this coroutine. Pass-by-reference is not permitted "
        "as the references may become invalid when the coroutine is suspending. This error also occurs if you try to "
        "define a member function returning TTask<> when the class it is defined in does not inherit from "
        "std::enable_shared_from_this. Coroutines can only be used on global functions and inside classes "
        "implementing std::enable_shared_from_this.");
    static_assert(
        (disjunction_v<negation<is_pointer<Args>>> && ...),
        "Detected pass-by-pointer for one or more arguments to this coroutine. Pass-by-pointer in coroutines is not "
        "permitted as they may become invalid when the coroutine is suspending.");
    static_assert(
        (negation_v<is_member_object_pointer<Args>> && ...),
        "Detected one or more arguments to this coroutine are a member object pointer. This is not supported.");
    static_assert(
        (negation_v<is_member_function_pointer<Args>> && ...),
        "Detected one or more arguments to this coroutine are a member function pointer. This is not supported.");
};

// Specialise coroutine_traits so the compiler knows the promise type to use.
namespace std
{

template <typename ReturnType, typename... Args>
struct coroutine_traits<TTask<ReturnType, ETaskBinding::Static>, Args...> : public TCoroutineAsserts<Args...>
{
    struct promise_type : TCoroutinePromise<ReturnType, ETaskBinding::Static>
    {
        promise_type()
            : TCoroutinePromise<ReturnType, ETaskBinding::Static>(std::make_shared<TTaskStateStatic<ReturnType>>())
        {
        }
    };
};

template <typename ClassType, typename ReturnType, typename... Args>
    requires(std::derived_from<ClassType, std::enable_shared_from_this<ClassType>>)
struct coroutine_traits<TTask<ReturnType, ETaskBinding::Unspecified>, ClassType &, Args...>
    : public TCoroutineAsserts<Args...>
{
    struct promise_type : TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>
    {
        promise_type(ClassType &Object)
            : TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>(
                  std::make_shared<TTaskStateSharedFromThis<ReturnType>>(Object.weak_from_this()))
        {
        }
    };
};

template <typename ClassType, typename ReturnType, typename... Args>
    requires(std::derived_from<ClassType, std::enable_shared_from_this<ClassType>>)
struct coroutine_traits<TTask<ReturnType, ETaskBinding::Unspecified>, ClassType &&, Args...>
    : public TCoroutineAsserts<Args...>
{
    struct promise_type : TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>
    {
        promise_type(ClassType &&Object)
            : TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>(
                  std::make_shared<TTaskStateSharedFromThis<ReturnType>>(Object.weak_from_this()))
        {
        }
    };
};

}