#include "AsyncMain.h"

#include "DelayOnAnotherThread.h"
#include <string>

// @notes:
//
// - TCoroutineTraits has been updated with Args... and ETaskBinding
// - TTask now has an implicit cast to cast away different ETaskBindings
// - TCoroutinePromise has ETaskBinding added to it's template, so that get_return_object can match the function return
//   exactly
// - Friend declaration on TTask has been updated with TaskBinding on TCoroutinePromise

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

    TTask<void> VoidCoroutine()
    {
        co_await FDelayOnAnotherThread{};
        co_return;
    }

    TTask<std::string> SimpleCoroutine(std::string Suffix)
    {
        co_await VoidCoroutine();
        co_return this->MyString + Suffix;
    }

    /*
    TTask<void> CoroutineWithPointer(int *Ptr)
    {
        co_return;
    }
    */
};

TTask<int, ETaskBinding::Static> AsyncMain()
{
    auto Object = std::make_shared<FMyObject>("hello world!");

    auto Task = Object->SimpleCoroutine(" with my suffix");

    // Object.reset();

    std::string Value = co_await Task;
    printf("got value from SimpleCoroutine: %s\n", Value.c_str());

    co_await GlobalCoroutine();

    co_return 0;
}