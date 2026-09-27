#pragma once

#include "CoroutineTraits.h"
#include "Task.h"

TTask<int, ETaskBinding::Static> AsyncMain();