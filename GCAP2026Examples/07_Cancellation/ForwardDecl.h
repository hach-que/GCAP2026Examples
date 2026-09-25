#pragma once

#include "TaskBinding.h"

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TCoroutinePromise;

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TTask;