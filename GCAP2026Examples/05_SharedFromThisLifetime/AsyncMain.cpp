#include "AsyncMain.h"

#include "CoroutineTraits.h"
#include "DelayOnAnotherThread.h"
#include <string>

struct FMyObject : std::enable_shared_from_this<FMyObject>
{
    // shared_ptr to really make this crash dereferencing invalid memory
    std::shared_ptr<std::string> MyString;

    FMyObject(const std::string &InMyString)
        : MyString(std::make_shared<std::string>(InMyString))
    {
    }

    ~FMyObject()
    {
        this->MyString.reset();
    }

    TTask<void> VoidCoroutine()
    {
        co_await FDelayOnAnotherThread{};
        co_return;
    }

    TTask<std::string> SimpleCoroutine()
    {
        co_await VoidCoroutine();
        co_return *this->MyString;
    }
};

TTask<int> AsyncMain()
{
    auto Object = std::make_shared<FMyObject>("hello world!");

    auto Task = Object->SimpleCoroutine();

    // Invalidate the object.
    Object.reset();

    std::string Value = co_await Task;

    printf("got value from SimpleCoroutine: %s\n", Value.c_str());

    co_return 0;
}