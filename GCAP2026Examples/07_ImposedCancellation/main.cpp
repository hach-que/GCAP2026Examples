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
    while (!MainTask.get_internal_task_state()->IsReadyToResume() && !MainTask.get_internal_task_state()->IsCancelled())
    {
        auto DispatchCopy = MainDispatch;
        MainDispatch.clear();
        for (const auto &ToDispatch : DispatchCopy)
        {
            ToDispatch();
        }

        std::this_thread::sleep_for(50ms);
    }

    if (MainTask.get_internal_task_state()->IsReadyToResume())
    {
        return MainTask.await_resume();
    }
    else
    {
        // Task was cancelled, so no result code.
        printf("AsyncMain task was cancelled.");
        return 1;
    }
}