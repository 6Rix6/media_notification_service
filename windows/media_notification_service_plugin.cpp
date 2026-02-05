#include "media_notification_service_plugin.h"

#include <windows.h>
#include <VersionHelpers.h>

#include <flutter/event_channel.h>
#include <flutter/event_stream_handler_functions.h>

#include <winrt/windows.foundation.h>
#include <winrt/Windows.Media.Control.h>

#include <memory>
#include <sstream>

using namespace winrt::Windows::Media::Control;

namespace media_notification_service
{

  // static
  void MediaNotificationServicePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto method_channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "com.example.media_notification_service/media",
            &flutter::StandardMethodCodec::GetInstance());

    auto event_channel =
        std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
            registrar->messenger(), "com.example.media_notification_service/media_stream",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<MediaNotificationServicePlugin>();

    method_channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    auto handler = std::make_unique<flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments,
                                        std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
            -> std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
        {
          return plugin_pointer->OnListenInternal(arguments, std::move(events));
        },
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments)
            -> std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
        {
          return plugin_pointer->OnCancelInternal(arguments);
        });

    event_channel->SetStreamHandler(std::move(handler));

    registrar->AddPlugin(std::move(plugin));
  }

  MediaNotificationServicePlugin::MediaNotificationServicePlugin()
      : stop_worker_(false)
  {
    worker_thread_ = std::thread(&MediaNotificationServicePlugin::WorkerThreadFunc, this);
  }

  MediaNotificationServicePlugin::~MediaNotificationServicePlugin()
  {
    EnqueueTask([this]()
                { RemoveEventListeners(); });

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      stop_worker_ = true;
    }
    queue_cv_.notify_one();

    if (worker_thread_.joinable())
    {
      worker_thread_.join();
    }
  }

  std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
  MediaNotificationServicePlugin::OnListenInternal(
      const flutter::EncodableValue *arguments,
      std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> &&events)
  {
    {
      std::lock_guard<std::mutex> lock(event_sink_mutex_);
      event_sink_ = std::move(events);
    }

    EnqueueTask([this]()
                {
      SetupEventListeners();
      OnMediaChanged(); });

    return nullptr;
  }

  std::unique_ptr<flutter::StreamHandlerError<flutter::EncodableValue>>
  MediaNotificationServicePlugin::OnCancelInternal(const flutter::EncodableValue *arguments)
  {
    EnqueueTask([this]()
                { RemoveEventListeners(); });

    {
      std::lock_guard<std::mutex> lock(event_sink_mutex_);
      event_sink_.reset();
    }

    return nullptr;
  }

  void MediaNotificationServicePlugin::WorkerThreadFunc()
  {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    try
    {
      media_manager_ =
          GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
    }
    catch (...)
    {
      media_manager_ = nullptr;
    }

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

  void MediaNotificationServicePlugin::EnqueueTask(Task task)
  {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      task_queue_.push(std::move(task));
    }
    queue_cv_.notify_one();
  }

  flutter::EncodableMap MediaNotificationServicePlugin::GetCurrentMediaInfo()
  {
    flutter::EncodableMap map;

    try
    {
      if (media_manager_)
      {
        auto session = media_manager_.GetCurrentSession();

        if (session)
        {
          auto props = session.TryGetMediaPropertiesAsync().get();
          map[flutter::EncodableValue("title")] =
              flutter::EncodableValue(winrt::to_string(props.Title()));
          map[flutter::EncodableValue("artist")] =
              flutter::EncodableValue(winrt::to_string(props.Artist()));
          map[flutter::EncodableValue("album")] =
              flutter::EncodableValue(winrt::to_string(props.AlbumTitle()));

          auto playback_info = session.GetPlaybackInfo();
          auto status = playback_info.PlaybackStatus();

          std::string playback_state;
          switch (status)
          {
          case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
            playback_state = "playing";
            break;
          case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
            playback_state = "paused";
            break;
          case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
            playback_state = "stopped";
            break;
          default:
            playback_state = "unknown";
            break;
          }

          map[flutter::EncodableValue("playbackState")] =
              flutter::EncodableValue(playback_state);
        }
      }
    }
    catch (...)
    {
    }

    return map;
  }

  void MediaNotificationServicePlugin::SetupEventListeners()
  {
    if (!media_manager_)
      return;

    try
    {
      sessions_changed_token_ = media_manager_.SessionsChanged(
          [this](auto &&, auto &&)
          {
            OnMediaChanged();
          });

      auto session = media_manager_.GetCurrentSession();
      if (session)
      {
        media_properties_changed_token_ = session.MediaPropertiesChanged(
            [this](auto &&, auto &&)
            {
              OnMediaChanged();
            });

        playback_info_changed_token_ = session.PlaybackInfoChanged(
            [this](auto &&, auto &&)
            {
              OnMediaChanged();
            });
      }
    }
    catch (...)
    {
    }
  }

  void MediaNotificationServicePlugin::RemoveEventListeners()
  {
    if (!media_manager_)
      return;

    try
    {
      if (sessions_changed_token_)
      {
        media_manager_.SessionsChanged(sessions_changed_token_);
        sessions_changed_token_ = {};
      }

      auto session = media_manager_.GetCurrentSession();
      if (session)
      {
        if (media_properties_changed_token_)
        {
          session.MediaPropertiesChanged(media_properties_changed_token_);
          media_properties_changed_token_ = {};
        }

        if (playback_info_changed_token_)
        {
          session.PlaybackInfoChanged(playback_info_changed_token_);
          playback_info_changed_token_ = {};
        }
      }
    }
    catch (...)
    {
    }
  }

  void MediaNotificationServicePlugin::OnMediaChanged()
  {
    auto map = GetCurrentMediaInfo();

    std::lock_guard<std::mutex> lock(event_sink_mutex_);
    if (event_sink_)
    {
      event_sink_->Success(flutter::EncodableValue(map));
    }
  }

  void MediaNotificationServicePlugin::HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    std::string name = method_call.method_name();

    if (name == "getCurrentMedia")
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      EnqueueTask([this, result = result_shared]()
                  {
        auto map = GetCurrentMediaInfo();
        result->Success(flutter::EncodableValue(map)); });

      return;
    }
    else if (name == "getQueue")
    {
      flutter::EncodableList list = {};
      result->Success(flutter::EncodableValue(list));
    }
    else if (name == "hasPermission")
    {
      result->Success(flutter::EncodableValue(true));
    }
    else
    {
      result->NotImplemented();
    }
  }

} // namespace media_notification_service