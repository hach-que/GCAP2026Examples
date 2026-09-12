#include "AsyncMain.h"

#include "CoroutineTraits.h"
#include "DelayOnAnotherThread.h"

static TTask<int> SimpleCoroutine()
{
    co_await FDelayOnAnotherThread{};

    co_return 5;
}

TTask<int> AsyncMain()
{
    int Value = co_await SimpleCoroutine();

    printf("got value from SimpleCoroutine: %d\n", Value);

    co_return 0;
}