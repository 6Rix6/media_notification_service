#ifndef FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_
#define FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/event_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>

#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace media_notification_service
{

    class MediaNotificationServicePlugin : public flutter::Plugin,
                                           public flutter::StreamHandler<flutter::EncodableValue>
    {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

        MediaNotificationServicePlugin();

        virtual ~MediaNotificationServicePlugin();

        // Disallow copy and assign.
        MediaNotificationServicePlugin(const MediaNotificationServicePlugin &) = delete;
        MediaNotificationServicePlugin &operator=(const MediaNotificationServicePlugin &) = delete;

        // StreamHandler overrides
        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnListenInternal(
            const flutter::EncodableValue *arguments,
            std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events) override;

        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnCancelInternal(
            const flutter::EncodableValue *arguments) override;

        using Task = std::function<void()>;
        void EnqueueTask(Task task);

    private:
        void WorkerThreadFunc();

        // method channel handler
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue> &method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

        flutter::EncodableMap GetCurrentMediaInfo();

        void SetupEventListeners();

        void RemoveEventListeners();

        void OnMediaChanged();

        std::thread worker_thread_;

        std::queue<Task> task_queue_;
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        bool stop_worker_;

        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager media_manager_{nullptr};

        std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
        std::mutex event_sink_mutex_;

        winrt::event_token sessions_changed_token_;
        winrt::event_token media_properties_changed_token_;
        winrt::event_token playback_info_changed_token_;
    };

} // namespace media_notification_service

#endif // FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_