#pragma once

#include "CoroutinePromise.h"
#include "Task.h"
#include <memory>

// Specialise coroutine_traits so the compiler knows the promise type to use.
namespace std
{

template <typename ReturnType>
struct coroutine_traits<TTask<ReturnType>>
{
    struct promise_type : TCoroutinePromise<ReturnType>
    {
        promise_type()
            : TCoroutinePromise<ReturnType>(std::make_shared<TTaskStateStatic<ReturnType>>())
        {
        }
    };
};

template <typename ClassType, typename ReturnType>
    requires(std::derived_from<ClassType, std::enable_shared_from_this<ClassType>>)
struct coroutine_traits<TTask<ReturnType>, ClassType &>
{
    struct promise_type : TCoroutinePromise<ReturnType>
    {
        promise_type(ClassType &Object)
            : TCoroutinePromise<ReturnType>(
                  std::make_shared<TTaskStateSharedFromThis<ReturnType>>(Object.weak_from_this()))
        {
        }
    };
};

template <typename ClassType, typename ReturnType>
    requires(std::derived_from<ClassType, std::enable_shared_from_this<ClassType>>)
struct coroutine_traits<TTask<ReturnType>, ClassType &&>
{
    struct promise_type : TCoroutinePromise<ReturnType>
    {
        promise_type(ClassType &&Object)
            : TCoroutinePromise<ReturnType>(
                  std::make_shared<TTaskStateSharedFromThis<ReturnType>>(Object.weak_from_this()))
        {
        }
    };
};

}