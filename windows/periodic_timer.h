#ifndef PERIODIC_TIMER_H_
#define PERIODIC_TIMER_H_

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

namespace media_notification_service
{
    class PeriodicTimer
    {
    public:
        using Callback = std::function<void()>;

        PeriodicTimer();
        ~PeriodicTimer();

        PeriodicTimer(const PeriodicTimer &) = delete;
        PeriodicTimer &operator=(const PeriodicTimer &) = delete;

        void Start(std::chrono::milliseconds interval, Callback callback);
        void Stop();

        bool IsRunning() const;

    private:
        void TimerThreadFunc();

        std::thread timer_thread_;
        std::mutex mutex_;
        std::condition_variable cv_;
        std::atomic<bool> running_;
        std::chrono::milliseconds interval_;
        Callback callback_;
    };

} // namespace media_notification_service

#endif // PERIODIC_TIMER_H_