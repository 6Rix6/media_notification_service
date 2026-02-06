#include "periodic_timer.h"

namespace media_notification_service
{
    PeriodicTimer::PeriodicTimer() : running_(false) {}

    PeriodicTimer::~PeriodicTimer()
    {
        Stop();
    }

    void PeriodicTimer::Start(std::chrono::milliseconds interval, Callback callback)
    {
        Stop();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            interval_ = interval;
            callback_ = callback;
            running_ = true;
        }

        timer_thread_ = std::thread(&PeriodicTimer::TimerThreadFunc, this);
    }

    void PeriodicTimer::Stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
        }
        cv_.notify_one();

        if (timer_thread_.joinable())
        {
            timer_thread_.join();
        }

        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = nullptr;
    }

    bool PeriodicTimer::IsRunning() const
    {
        return running_;
    }

    void PeriodicTimer::TimerThreadFunc()
    {
        while (running_)
        {
            auto start_time = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (callback_ && running_)
                {
                    callback_();
                }
            }

            std::unique_lock<std::mutex> lock(mutex_);
            auto next_time = start_time + interval_;
            cv_.wait_until(lock, next_time, [this]
                           { return !running_; });
        }
    }

} // namespace media_notification_service