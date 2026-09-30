#pragma once

#include <memory>
#include <optional>
#include <variant>

struct FTaskLifetimeBindingStatic
{
};

struct FTaskLifetimeBindingSharedFromThis
{
    std::weak_ptr<void> Owner;
};

struct FTaskLifetimeBindingLate
{
    std::optional<std::weak_ptr<const void>> Owner;
};

using FTaskLifetimeBinding =
    std::variant<FTaskLifetimeBindingStatic, FTaskLifetimeBindingSharedFromThis, FTaskLifetimeBindingLate>;

extern bool IsBound(const FTaskLifetimeBinding &Lifetime) noexcept;

template <typename ClassType>
void TryBindToSharedPtr(FTaskLifetimeBinding &Lifetime, const std::weak_ptr<const ClassType> &InObject) noexcept
{
    if (Lifetime.index() == 2)
    {
        std::get<2>(Lifetime).Owner =
            std::weak_ptr<const void>(std::static_pointer_cast<const void, const ClassType>(InObject.lock()));
    }
}