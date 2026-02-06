#include "stream_controller.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

namespace media_notification_service
{
    StreamController::StreamController() = default;

    StreamController::~StreamController() = default;

    void StreamController::RegisterEventChannel(
        flutter::PluginRegistrarWindows *registrar,
        const std::string &channel_name,
        OnListenCallback on_listen,
        OnCancelCallback on_cancel)
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

    void StreamController::Send(const flutter::EncodableValue &value)
    {
        std::lock_guard<std::mutex> lock(sink_mutex_);
        if (event_sink_)
        {
            event_sink_->Success(value);
        }
    }

    void StreamController::SendError(const std::string &error_code, const std::string &error_message)
    {
        std::lock_guard<std::mutex> lock(sink_mutex_);
        if (event_sink_)
        {
            event_sink_->Error(error_code, error_message);
        }
    }

    std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
    StreamController::OnListen(
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

    std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
    StreamController::OnCancel(const flutter::EncodableValue *arguments)
    {
        std::lock_guard<std::mutex> lock(sink_mutex_);
        event_sink_.reset();

        if (on_cancel_callback_)
        {
            on_cancel_callback_(arguments);
        }

        return nullptr;
    }

} // namespace media_notification_service