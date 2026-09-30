#include "DelayOnAnotherThread.h"

#include "MainDispatch.h"
#include <coroutine>
#include <iostream>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

static void WaitAndResume(std::coroutine_handle<> Handle)
{
    // printf("FDelayOnAnotherThread sleeping for 1 second on a different thread\n");
    std::this_thread::sleep_for(1000ms);

    // Use MainDispatch to make this run on the main thread again.
    MainDispatch.push_back([Handle]() {
        // printf("FDelayOnAnotherThread now resuming execution on main thread\n");
        Handle.resume();
    });
}

bool FDelayOnAnotherThread::await_ready() noexcept
{
    // We will always suspend.
    return false;
}

void FDelayOnAnotherThread::await_suspend(std::coroutine_handle<> Handle)
{
    auto Thread = std::thread(WaitAndResume, Handle);
    Thread.detach();
}

void FDelayOnAnotherThread::await_resume()
{
    // Nothing is returned from co_await'ing this value.
}

static void WaitAndResumeDeferred(std::chrono::milliseconds DurationMs, TDeferredTask<void> Deferred)
{
    // std::ostringstream out;
    // out << "DelayOnAnotherThread() sleeping for " << DurationMs << " on a different thread";
    // printf("%s\n", out.str().c_str());
    std::this_thread::sleep_for(DurationMs);

    // Use MainDispatch to make this run on the main thread again.
    MainDispatch.push_back([Deferred]() {
        // printf("DelayOnAnotherThread() now resuming execution on main thread\n");
        Deferred.SetValue();
    });
}

TTask<void> DelayOnAnotherThread(std::chrono::milliseconds DurationMs)
{
    auto Deferred = TTask<void>::Deferred();

    auto Thread = std::thread(WaitAndResumeDeferred, DurationMs, Deferred);
    Thread.detach();

    return Deferred;
}