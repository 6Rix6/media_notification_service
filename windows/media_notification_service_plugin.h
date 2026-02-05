#ifndef FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_
#define FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include <winrt/Windows.Media.Control.h>

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace media_notification_service
{

    class MediaNotificationServicePlugin : public flutter::Plugin
    {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

        MediaNotificationServicePlugin();

        virtual ~MediaNotificationServicePlugin();

        // Disallow copy and assign.
        MediaNotificationServicePlugin(const MediaNotificationServicePlugin &) = delete;
        MediaNotificationServicePlugin &operator=(const MediaNotificationServicePlugin &) = delete;

    private:
        using Task = std::function<void()>;

        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue> &method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

        void WorkerThreadFunc();

        void EnqueueTask(Task task);

        // worker thread
        std::thread worker_thread_;

        // task queue
        std::queue<Task> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        bool stop_worker_;

        // media controller
        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager media_manager_{nullptr};
    };

} // namespace media_notification_service

#endif // FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_