#include "stream_controller.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

namespace media_notification_service
{
    static const UINT WM_STREAM_EVENT = WM_USER + 1;

    StreamController::StreamController()
        : message_window_(nullptr) {}

    StreamController::~StreamController()
    {
        if (message_window_)
        {
            DestroyWindow(message_window_);
            message_window_ = nullptr;
        }
    }

    LRESULT CALLBACK StreamController::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (msg == WM_STREAM_EVENT)
        {
            StreamController *self = reinterpret_cast<StreamController *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self)
            {
                self->ProcessPendingEvents();
            }
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    void StreamController::ProcessPendingEvents()
    {
        std::lock_guard<std::mutex> lock(sink_mutex_);
        while (!pending_events_.empty() && event_sink_)
        {
            event_sink_->Success(pending_events_.front());
            pending_events_.pop();
        }
    }

    void StreamController::RegisterEventChannel(
        flutter::PluginRegistrarWindows *registrar,
        const std::string &channel_name,
        OnListenCallback on_listen,
        OnCancelCallback on_cancel)
    {
        on_listen_callback_ = on_listen;
        on_cancel_callback_ = on_cancel;

        // Create a message-only window for thread marshalling
        WNDCLASS wc = {};
        wc.lpfnWndProc = StreamController::WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"StreamControllerMessageWindow";
        RegisterClass(&wc);

        message_window_ = CreateWindowEx(
            0, L"StreamControllerMessageWindow", L"", 0,
            0, 0, 0, 0, HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), nullptr);

        SetWindowLongPtr(message_window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

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
        {
            std::lock_guard<std::mutex> lock(sink_mutex_);
            pending_events_.push(value);
        }
        if (message_window_)
        {
            PostMessage(message_window_, WM_STREAM_EVENT, 0, 0);
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