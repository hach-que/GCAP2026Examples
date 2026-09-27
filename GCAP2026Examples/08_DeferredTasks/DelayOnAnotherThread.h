#pragma once

#include "Task.h"
#include <coroutine>

struct FDelayOnAnotherThread
{
    bool await_ready() noexcept;
    void await_suspend(std::coroutine_handle<> Handle);
    void await_resume();
};

// --- Delay on another thread implemented as a deferred task

TTask<void> DelayOnAnotherThread();