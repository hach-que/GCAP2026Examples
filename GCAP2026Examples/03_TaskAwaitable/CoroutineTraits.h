#pragma once

#include "CoroutinePromise.h"
#include "Task.h"

// Specialise coroutine_traits so the compiler knows the promise type to use.
namespace std
{

template <typename ReturnType>
struct coroutine_traits<TTask<ReturnType>>
{
    using promise_type = ::TCoroutinePromise<ReturnType>;
};

}