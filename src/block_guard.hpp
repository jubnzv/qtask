#pragma once

#include <QObject>

#include <tuple>

/// @brief Blocks signals for the given QObject pointers until the guard is
/// destroyed.
/// @param args A list of QObject pointers. The signals for these objects will
/// be blocked.
/// @return A RAII object that unblocks the signals when it goes out of scope.
template <typename... Ptrs>
auto BlockGuard(Ptrs... args)
{
    static_assert((std::is_pointer_v<Ptrs> && ...),
                  "All arguments must be raw pointers.");
    class SignalBlocker {
      private:
        std::tuple<Ptrs...> pointers;

      public:
        SignalBlocker(const SignalBlocker &) = delete;
        SignalBlocker &operator=(const SignalBlocker &) = delete;
        SignalBlocker(SignalBlocker &&) = delete;
        SignalBlocker &operator=(SignalBlocker &&) = delete;

        explicit SignalBlocker(Ptrs... args)
            : pointers(args...)
        {
            std::apply([](Ptrs &...args) { (..., args->blockSignals(true)); },
                       pointers);
        }
        ~SignalBlocker()
        {
            std::apply([](Ptrs &...args) { (..., args->blockSignals(false)); },
                       pointers);
        }
    };
    return SignalBlocker(args...);
}
