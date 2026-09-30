#pragma once

class IUnbound
{
protected:
    IUnbound() = default;

public:
    IUnbound(const IUnbound &) = default;
    IUnbound(IUnbound &&) = default;
    ~IUnbound() = default;
};