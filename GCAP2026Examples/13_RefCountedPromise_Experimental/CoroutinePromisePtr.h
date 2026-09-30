#pragma once

#include "DebugPrintf.h"
#include "ForwardDecl.h"
#include <utility>

template <typename ReturnType>
struct TCoroutinePromisePtr
{
    friend TCoroutinePromise<ReturnType, ETaskBinding::Unspecified>;
    friend TCoroutinePromise<ReturnType, ETaskBinding::Static>;
    friend TCoroutinePromise<ReturnType, ETaskBinding::Unbound>;
    using PromiseType = TCoroutinePromiseBase<ReturnType>;

private:
    PromiseType *Promise;

    TCoroutinePromisePtr(PromiseType *InPromise)
        : Promise(InPromise)
    {
        DEBUG_PRINTF("%p captured for TCoroutinePromisePtr with ref count %d\n", InPromise, InPromise->RefCount);
    }

public:
    TCoroutinePromisePtr(const TCoroutinePromisePtr &Other) noexcept
        : Promise(Other.Promise)
    {
        this->IncreaseRef(this->Promise);
    }
    TCoroutinePromisePtr(TCoroutinePromisePtr &&Other) noexcept
        : Promise(std::exchange(Other.Promise, nullptr))
    {
    }
    ~TCoroutinePromisePtr()
    {
        this->DecreaseRef(this->Promise);
    }
    TCoroutinePromisePtr &operator=(const TCoroutinePromisePtr &Other)
    {
        if (this->Promise != Other.Promise)
        {
            this->IncreaseRef(this->Promise);
            this->DecreaseRef(std::exchange(this->Promise, Other.Promise));
        }
        return *this;
    }
    TCoroutinePromisePtr &operator=(TCoroutinePromisePtr &&Other)
    {
        if (this->Promise != Other.Promise)
        {
            this->DecreaseRef(std::exchange(this->Promise, std::exchange(Other.Promise, nullptr)));
        }
        return *this;
    }
    PromiseType *operator->() const noexcept
    {
        return this->Promise;
    }

private:
    static void IncreaseRef(PromiseType *Promise) noexcept
    {
        if (Promise)
        {
            Promise->IncreaseRef();
        }
    }

    static void DecreaseRef(PromiseType *Promise)
    {
        if (Promise)
        {
            Promise->DecreaseRef();
        }
    }
};
