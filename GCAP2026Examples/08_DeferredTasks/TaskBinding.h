#pragma once

#include <cstdint>

enum class ETaskBinding : uint8_t
{
    // Default binding on objects.
    Unspecified,

    // Binding for static functions.
    Static,

    // Binding for unbound lambdas that can capture.
    Unbound,
};