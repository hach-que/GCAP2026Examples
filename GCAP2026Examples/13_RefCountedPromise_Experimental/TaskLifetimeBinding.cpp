#include "TaskLifetimeBinding.h"

bool IsBound(const FTaskLifetimeBinding &Lifetime) noexcept
{
    if (Lifetime.index() == 0)
    {
        return true;
    }
    else if (Lifetime.index() == 1)
    {
        return !std::get<1>(Lifetime).Owner.expired();
    }
    else if (Lifetime.index() == 2)
    {
        return !std::get<2>(Lifetime).Owner.has_value() || !std::get<2>(Lifetime).Owner.value().expired();
    }

    return false;
}