#pragma once

#include "TaskBinding.h"

template <typename ReturnType>
struct TCoroutinePromisePtr;

template <typename ReturnType>
struct TCoroutinePromiseBase;

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TCoroutinePromise;

template <typename ReturnType, ETaskBinding TaskBinding = ETaskBinding::Unspecified>
struct TTask;

template <typename ReturnType>
struct TTaskState;

template <typename ReturnType>
struct TDeferredTask;