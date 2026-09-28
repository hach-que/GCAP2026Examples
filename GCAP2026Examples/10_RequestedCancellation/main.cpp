#include <chrono>
#include <coroutine>
#include <memory>
#include <thread>

#include "AsCallback.h"
#include "AsyncMain.h"
#include "MainDispatch.h"

// Call the AsyncMain coroutine.
int main()
{
    using namespace std::chrono_literals;

    std::optional<int> ExitCode;
    AsCallback(
        AsyncMain(),
        [&ExitCode](int InExitCode) {
            ExitCode = InExitCode;
        },
        [&ExitCode] {
            printf("AsyncMain task was cancelled.");
            ExitCode = 1;
        });

    while (!ExitCode.has_value())
    {
        auto DispatchCopy = MainDispatch;
        MainDispatch.clear();
        for (const auto &ToDispatch : DispatchCopy)
        {
            ToDispatch();
        }

        std::this_thread::sleep_for(50ms);
    }

    return ExitCode.value();
}