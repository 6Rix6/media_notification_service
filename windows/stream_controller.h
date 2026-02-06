#ifndef STREAM_CONTROLLER_H_
#define STREAM_CONTROLLER_H_

#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <mutex>
#include <memory>
#include <functional>

namespace media_notification_service
{
    class StreamController
    {
    public:
        using OnListenCallback = std::function<void(const flutter::EncodableValue *arguments)>;
        using OnCancelCallback = std::function<void(const flutter::EncodableValue *arguments)>;

        StreamController() = default;
        ~StreamController() = default;

        StreamController(const StreamController &) = delete;
        StreamController &operator=(const StreamController &) = delete;

        void RegisterEventChannel(
            flutter::PluginRegistrarWindows *registrar,
            const std::string &channel_name,
            OnListenCallback on_listen = nullptr,
            OnCancelCallback on_cancel = nullptr)
        {
            on_listen_callback_ = on_listen;
            on_cancel_callback_ = on_cancel;

            event_channel_ = std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
                registrar->messenger(),
                channel_name,
                &flutter::StandardMethodCodec::GetInstance());

            auto handler = std::make_unique<flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
                [this](const flutter::EncodableValue *arguments,
                       std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
                {
                    return this->OnListen(arguments, std::move(events));
                },
                [this](const flutter::EncodableValue *arguments)
                {
                    return this->OnCancel(arguments);
                });

            event_channel_->SetStreamHandler(std::move(handler));
        }

        void Send(const flutter::EncodableValue &value)
        {
            std::lock_guard<std::mutex> lock(sink_mutex_);
            if (event_sink_)
            {
                event_sink_->Success(value);
            }
        }

        void SendError(const std::string &error_code, const std::string &error_message)
        {
            std::lock_guard<std::mutex> lock(sink_mutex_);
            if (event_sink_)
            {
                event_sink_->Error(error_code, error_message);
            }
        }

    private:
        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnListen(
            const flutter::EncodableValue *arguments,
            std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
        {
            std::lock_guard<std::mutex> lock(sink_mutex_);
            event_sink_ = std::move(events);

            if (on_listen_callback_)
            {
                on_listen_callback_(arguments);
            }

            return nullptr;
        }

        std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>> OnCancel(
            const flutter::EncodableValue *arguments)
        {
            std::lock_guard<std::mutex> lock(sink_mutex_);
            event_sink_.reset();

            if (on_cancel_callback_)
            {
                on_cancel_callback_(arguments);
            }

            return nullptr;
        }

        std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>> event_channel_;
        std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
        std::mutex sink_mutex_;

        OnListenCallback on_listen_callback_;
        OnCancelCallback on_cancel_callback_;
    };

} // namespace media_notification_service

#endif // STREAM_CONTROLLER_H_