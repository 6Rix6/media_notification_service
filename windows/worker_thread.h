#ifndef WORKER_THREAD_H_
#define WORKER_THREAD_H_

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace media_notification_service
{
    class WorkerThread
    {
    public:
        using Task = std::function<void()>;

        WorkerThread();
        ~WorkerThread();

        WorkerThread(const WorkerThread &) = delete;
        WorkerThread &operator=(const WorkerThread &) = delete;

        void EnqueueTask(Task task);

        void Stop();

    private:
        void WorkerThreadFunc();

        std::thread thread_;
        std::queue<Task> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        bool stop_worker_;
    };

} // namespace media_notification_service

#endif // WORKER_THREAD_H_