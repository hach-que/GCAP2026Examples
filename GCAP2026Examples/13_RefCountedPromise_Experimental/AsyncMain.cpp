#include "AsyncMain.h"

#include "AsPotentiallyCancelled.h"
#include "Bind.h"
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

TTask<void> DeferredCancelExample()
{
    // @note: We don't ever do SetValue, so this deferred task won't ever complete normally. It has to have cancellation
    // requested.

    auto Deferred = TTask<void>::Deferred();
    Deferred.OnCancellationRequested([Deferred]() {
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
    {
        auto Object = std::make_shared<FMyObject>("hello world!");
        auto Task = Object->SimpleCoroutine(" with my suffix");

        Object.reset();

        std::optional<std::string> PotentiallyCancelled = co_await AsPotentiallyCancelled(Task);
        if (PotentiallyCancelled.has_value())
        {
            printf("EXAMPLE 1: got value from SimpleCoroutine: %s\n", PotentiallyCancelled.value().c_str());
        }
        else
        {
            printf("EXAMPLE 1: SimpleCoroutine was cancelled.\n");
        }
    }

    // --- EXAMPLE 2
    {
        auto Object = std::make_shared<FMyObject>("hello world!");
        auto Task = Object->SimpleCoroutine(" with my suffix");

        CancelAfter(Task, 500ms);

        std::optional<std::string> PotentiallyCancelled = co_await AsPotentiallyCancelled(Task);
        if (PotentiallyCancelled.has_value())
        {
            printf("EXAMPLE 2: got value from SimpleCoroutine: %s\n", PotentiallyCancelled.value().c_str());
        }
        else
        {
            printf("EXAMPLE 2: SimpleCoroutine was cancelled.\n");
        }
    }

    // --- EXAMPLE 3
    {
        auto Task = DeferredCancelExample();

        CancelAfter(Task, 500ms);

        EVoidPotentiallyCancelled PotentiallyCancelled = co_await AsPotentiallyCancelled(Task);
        if (PotentiallyCancelled == EVoidPotentiallyCancelled::Returned)
        {
            printf("EXAMPLE 3: DeferredCancelExample returned.\n");
        }
        else
        {
            printf("EXAMPLE 3: DeferredCancelExample was cancelled.\n");
        }
    }

    // --- EXAMPLE 4
    {
        auto Object = std::make_shared<FMyObject>("hello world!");

        auto UnboundLambda =
            [Ptr = Object.get()](IUnbound, std::string Suffix) -> TTask<std::string, ETaskBinding::Unbound> {
            printf("async lambda delay starting\n");
            co_await DelayOnAnotherThread(1000ms);
            printf("async lambda delay complete\n");
            co_return Ptr->MyString + Suffix;
        };

        auto BoundLambda = Bind(Object, UnboundLambda);
        // auto BoundLambda = Bind(UnboundLambda);

        auto Task = BoundLambda(" with my suffix");

        // Object.reset();

        std::optional<std::string> PotentiallyCancelled = co_await AsPotentiallyCancelled(Task);
        if (PotentiallyCancelled.has_value())
        {
            printf("EXAMPLE 4: got value from bound async lambda: %s\n", PotentiallyCancelled.value().c_str());
        }
        else
        {
            printf("EXAMPLE 4: bound async lambda was cancelled.\n");
        }
    }

    co_return 0;
}