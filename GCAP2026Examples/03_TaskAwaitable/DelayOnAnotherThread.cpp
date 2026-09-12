#include "DelayOnAnotherThread.h"

#include "MainDispatch.h"
#include <chrono>
#include <coroutine>
#include <thread>

using namespace std::chrono_literals;

static void WaitAndResume(std::coroutine_handle<> Handle)
{
    printf("FDelayOnAnotherThread sleeping for 1 second on a different thread\n");
    std::this_thread::sleep_for(1000ms);

    // Use MainDispatch to make this run on the main thread again.
    MainDispatch.push_back([Handle]() {
        printf("FDelayOnAnotherThread now resuming execution on main thread\n");
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