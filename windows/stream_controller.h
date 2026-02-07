#ifndef STREAM_CONTROLLER_H_
#define STREAM_CONTROLLER_H_

#include <flutter/event_channel.h>
#include <flutter/encodable_value.h>
#include <mutex>
#include <memory>
#include <functional>
#include <queue>
#include <windows.h>
#include <string>

namespace flutter
{
    class PluginRegistrarWindows;
}

namespace media_notification_service
{
    class StreamController
    {
    public:
        using OnListenCallback = std::function<void(const flutter::EncodableValue *arguments)>;
        using OnCancelCallback = std::function<void(const flutter::EncodableValue *arguments)>;

        StreamController();
        ~StreamController();

        StreamController(const StreamController &) = delete;
        StreamController &operator=(const StreamController &) = delete;

        void RegisterEventChannel(
            flutter::PluginRegistrarWindows *registrar,
            const std::string &channel_name,
            OnListenCallback on_listen = nullptr,
            OnCancelCallback on_cancel = nullptr);

        void Send(const flutter::EncodableValue &value);
        void SendError(const std::string &error_code, const std::string &error_message);

    private:
        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnListen(
            const flutter::EncodableValue *arguments,
            std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events);

        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnCancel(
            const flutter::EncodableValue *arguments);

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void ProcessPendingEvents();

        std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>> event_channel_;
        std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
        std::mutex sink_mutex_;
        std::queue<flutter::EncodableValue> pending_events_;

        HWND message_window_;

        OnListenCallback on_listen_callback_;
        OnCancelCallback on_cancel_callback_;
    };

} // namespace media_notification_service

#endif // STREAM_CONTROLLER_H_