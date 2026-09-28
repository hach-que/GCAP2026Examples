#pragma once

#include "Task.h"
#include <chrono>
#include <coroutine>

struct FDelayOnAnotherThread
{
    bool await_ready() noexcept;
    void await_suspend(std::coroutine_handle<> Handle);
    void await_resume();
};

TTask<void> DelayOnAnotherThread(std::chrono::milliseconds Duration);