#pragma once

#include <coroutine>

struct FDelayOnAnotherThread
{
    bool await_ready() noexcept;
    void await_suspend(std::coroutine_handle<> Handle);
    void await_resume();
};