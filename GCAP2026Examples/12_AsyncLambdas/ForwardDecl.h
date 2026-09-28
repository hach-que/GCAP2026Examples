#pragma once

#include "TaskBinding.h"

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TCoroutinePromiseBase;

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TTask;

template <typename ReturnType>
struct TDeferredTask;