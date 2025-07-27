#ifndef CPPXX_MULTITHREADING_CHANNEL_H
#define CPPXX_MULTITHREADING_CHANNEL_H

#include <queue>
#include <future>
#include <atomic>
#include <semaphore>


namespace cppxx::multithreading {
    template <typename T>
    class Channel {
    public:
        static_assert(!std::is_reference_v<T>, "T must not be a reference type");

        explicit Channel(int n)
            : futures(),
              sem(n) {}

        ~Channel() {
            for (; !futures.empty(); futures.pop())
                std::ignore = futures.front().get();
        }

        template <typename F>
            requires std::invocable<F> && std::same_as<std::invoke_result_t<F>, T>
        void operator<<(F &&f) {
            if (terminated)
                return;
            sem.acquire();
            futures.push(std::async(std::launch::async, [this, f = std::forward<F>(f)]() -> T {
                auto _ = SemaphoreReleaser(sem);
                return f();
            }));
        }

        template <typename F>
            requires std::invocable<F, T>
        void operator>>(F &&f) {
            f(futures.front().get());
            futures.pop();
        }

        bool empty() const { return futures.empty(); }
        void terminate() { terminated = true; }

    protected:
        std::queue<std::future<T>> futures;
        std::counting_semaphore<8> sem;
        std::atomic_bool terminated;

        struct SemaphoreReleaser {
            std::counting_semaphore<8> &sem;
            ~SemaphoreReleaser() { sem.release(); }
        };
    };
} // namespace onnxwrapper::multithreading

#endif
