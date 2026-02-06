#ifndef FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_
#define FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>

#include "stream_controller.h"
#include "media_session_manager.h"
#include "worker_thread.h"
#include "periodic_timer.h"

#include <memory>
#include <optional>
#include <windows.h>

namespace media_notification_service
{
    class MediaNotificationServicePlugin : public flutter::Plugin
    {
    public:
        static void RegisterWithRegistrar(flutter::PluginRegistrarWindows *registrar);

        MediaNotificationServicePlugin();
        virtual ~MediaNotificationServicePlugin();

        MediaNotificationServicePlugin(const MediaNotificationServicePlugin &) = delete;
        MediaNotificationServicePlugin &operator=(const MediaNotificationServicePlugin &) = delete;

    private:
        void HandleMethodCall(
            const flutter::MethodCall<flutter::EncodableValue> &method_call,
            std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result);

        void OnMediaChanged();
        void OnPositionChanged();

        WorkerThread worker_thread_;
        MediaSessionManager media_session_manager_;

        StreamController media_stream_handler_;
        StreamController position_stream_handler_;
        StreamController queue_stream_handler_;

        PeriodicTimer position_timer_;
    };

} // namespace media_notification_service

#endif // FLUTTER_PLUGIN_MEDIA_NOTIFICATION_SERVICE_PLUGIN_H_