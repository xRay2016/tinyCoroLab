#pragma once

#include "coro/concepts/awaitable.hpp"
#include <concepts>

namespace coro::concepts
{
template<typename T>
concept lockType = requires(T mtx) {
    { mtx.lock() } -> awaiter;
    { mtx.unlock() } -> std::same_as<void>;
};
}; // namespace coro::concepts