#include "AsyncMain.h"

#include "DelayOnAnotherThread.h"
#include <chrono>
#include <string>

using namespace std::chrono_literals;

TTask<void, ETaskBinding::Static> GlobalCoroutine()
{
    co_return;
}

struct FMyObject : std::enable_shared_from_this<FMyObject>
{
    std::string MyString;

    FMyObject(const std::string &InMyString)
        : MyString(InMyString)
    {
    }

    TTask<void> VoidCoroutine(std::chrono::milliseconds Duration)
    {
        co_await DelayOnAnotherThread(Duration);
        co_return;
    }

    TTask<std::string> SimpleCoroutine(std::string Suffix)
    {
        co_await VoidCoroutine(1000ms);
        co_return this->MyString + Suffix;
    }
};

TTask<std::string> DeferredCancelExample()
{
    // @note: We don't ever do SetValue, so this deferred task won't ever complete normally. It has to have cancellation
    // requested.

    auto Deferred = TTask<std::string>::Deferred();
    Deferred.OnCancellationRequested([Deferred]() mutable {
        printf("deferred task got cancellation request!\n");
        Deferred.SetCancelled();
    });
    return Deferred;
}

template <typename ReturnType>
TTask<void, ETaskBinding::Static> CancelAfter(TTask<ReturnType> Task, std::chrono::milliseconds Duration)
{
    co_await DelayOnAnotherThread(Duration);
    Task.RequestCancellation();
    co_return;
}

TTask<int, ETaskBinding::Static> AsyncMain()
{
    // --- EXAMPLE 1
    auto Object = std::make_shared<FMyObject>("hello world!");
    auto Task = Object->SimpleCoroutine(" with my suffix");

    // --- EXAMPLE 2
    // auto Task = DeferredCancelExample();

    // Start cancelling after 500ms.
    CancelAfter(Task, 500ms);

    std::string Value = co_await Task;
    printf("got value from SimpleCoroutine: %s\n", Value.c_str());

    co_await GlobalCoroutine();

    co_return 0;
}