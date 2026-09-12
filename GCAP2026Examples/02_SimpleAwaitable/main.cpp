#include <chrono>
#include <coroutine>
#include <functional>
#include <memory>
#include <thread>
#include <variant>

using namespace std::chrono_literals;

// -------- AWAITABLE START --------

struct FDelayOnAnotherThread
{
    bool await_ready() noexcept
    {
        // We will always suspend.
        return false;
    }

    void await_suspend(std::coroutine_handle<> Handle)
    {
        auto Thread = std::thread(WaitAndResume, Handle);
        Thread.detach();
    }

    void await_resume()
    {
        // Nothing is returned from co_await'ing this value.
    }

private:
    static void WaitAndResume(std::coroutine_handle<> Handle)
    {
        printf("FDelayOnAnotherThread sleeping for 1 second on a different thread\n");
        std::this_thread::sleep_for(1000ms);

        // @note: The coroutine will continue execution on this other thread! We are not re-joining the main thread.
        printf("FDelayOnAnotherThread now resuming execution\n");
        Handle.resume();
    }
};

// -------- AWAITABLE END --------

// Our shared state that we're going to reference from our return type.
template <typename ReturnType>
struct TTaskState
{
    // The returned value.
    std::optional<ReturnType> Value;

    // A callback that can be set to receive the value when it's ready.
    std::function<void(ReturnType)> Callback;

    // Sets up a basic callback that just sets 'Value', in case the coroutine returns a value before the receiver of a
    // TTask has overridden the callback.
    TTaskState()
        : Value()
        , Callback([this](ReturnType InValue) {
            this->Value = InValue;
        })
    {
    }
};

// Our return type that we're going to use for our coroutines, referencing the state shared between the coroutine
// "promise" and callers who want to await.
template <typename ReturnType>
struct TTask
{
    // Shared state.
    std::shared_ptr<TTaskState<ReturnType>> State;
};

// Our "promise type" that specifies how our coroutine will behave.
template <typename ReturnType>
struct TCoroutinePromise
{
    std::shared_ptr<TTaskState<ReturnType>> State;

    TCoroutinePromise()
        : State(std::make_shared<TTaskState<ReturnType>>())
    {
    }

    std::suspend_never initial_suspend() noexcept
    {
        return {};
    }

    std::suspend_never final_suspend() noexcept
    {
        return {};
    }

    void unhandled_exception()
    {
    }

    void return_value(ReturnType Value)
    {
        this->State->Callback(Value);
    }

    TTask<ReturnType> get_return_object()
    {
        return TTask<ReturnType>{this->State};
    }
};

// Specialise coroutine_traits so the compiler knows the promise type to use.
namespace std
{

template <typename ReturnType>
struct coroutine_traits<TTask<ReturnType>>
{
    using promise_type = ::TCoroutinePromise<ReturnType>;
};

}

TTask<int> SimpleCoroutine()
{
    // ------ AWAITABLE USED
    printf("co_await delay inside coroutine\n");
    co_await FDelayOnAnotherThread{};
    printf("resuming after co_await\n");
    // ------

    co_return 5;
}

// Call and use the coroutine.
int main()
{
    bool bComplete = false;

    TTask<int> Task = SimpleCoroutine();
    if (Task.State->Value.has_value())
    {
        // Value immediately available.
        printf("Immediate: %d\n", Task.State->Value.value());
        bComplete = true;
    }
    else
    {
        Task.State->Callback = [Task, &bComplete](int InValue) {
            Task.State->Value =
                InValue; // Mirror default behaviour because we're keeping the API surface simple in this example.

            printf("Delayed: %d\n", Task.State->Value.value());
            bComplete = true;
        };
    }

    // Wait for the coroutine to be complete.
    while (!bComplete)
    {
        std::this_thread::sleep_for(50ms);
    }

    return 0;
}