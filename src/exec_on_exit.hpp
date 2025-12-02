#pragma once

#include <utility>

/// @brief Execs callable when goes out of scope.
template <typename taCallable>
class exec_on_exit final {
  public:
    exec_on_exit() = delete;
    exec_on_exit(const exec_on_exit &) = delete;
    exec_on_exit(exec_on_exit &&) = default;
    exec_on_exit &operator=(const exec_on_exit &) = delete;
    exec_on_exit &operator=(exec_on_exit &&) = default;

    explicit exec_on_exit(taCallable &&func) noexcept
        : func(std::move(func))
    {
    }

    ~exec_on_exit() { func(); }

  private:
    taCallable func;
};
