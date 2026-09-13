#include <chrono>
#include <coroutine>
#include <memory>
#include <thread>

#include "AsyncMain.h"
#include "MainDispatch.h"

// Call the AsyncMain coroutine.
int main()
{
    using namespace std::chrono_literals;

    bool bComplete = false;

    TTask<int> MainTask = AsyncMain();
    while (!MainTask.await_ready())
    {
        auto DispatchCopy = MainDispatch;
        MainDispatch.clear();
        for (const auto &ToDispatch : DispatchCopy)
        {
            ToDispatch();
        }

        std::this_thread::sleep_for(50ms);
    }

    return MainTask.await_resume();
}