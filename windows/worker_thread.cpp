#include "worker_thread.h"
#include <winrt/Windows.Foundation.h>

namespace media_notification_service
{
    WorkerThread::WorkerThread() : stop_worker_(false)
    {
        thread_ = std::thread(&WorkerThread::WorkerThreadFunc, this);
    }

    WorkerThread::~WorkerThread()
    {
        Stop();
    }

    void WorkerThread::EnqueueTask(Task task)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            task_queue_.push(std::move(task));
        }
        queue_cv_.notify_one();
    }

    void WorkerThread::Stop()
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_worker_ = true;
        }
        queue_cv_.notify_one();

        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    void WorkerThread::WorkerThreadFunc()
    {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        while (true)
        {
            Task task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                queue_cv_.wait(lock, [this]
                               { return stop_worker_ || !task_queue_.empty(); });

                if (stop_worker_ && task_queue_.empty())
                {
                    break;
                }

                if (!task_queue_.empty())
                {
                    task = std::move(task_queue_.front());
                    task_queue_.pop();
                }
            }

            if (task)
            {
                task();
            }
        }

        winrt::uninit_apartment();
    }

} // namespace media_notification_service