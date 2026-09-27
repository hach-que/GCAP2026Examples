#pragma once

#include <functional>
#include <vector>

// A simple list of callbacks to run on the main thread while waiting for coroutines to finish.
extern std::vector<std::function<void()>> MainDispatch;