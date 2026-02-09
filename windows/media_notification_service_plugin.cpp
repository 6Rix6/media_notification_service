#include "media_notification_service_plugin.h"

#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>

namespace media_notification_service
{
  void MediaNotificationServicePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto plugin = std::make_unique<MediaNotificationServicePlugin>();

    auto method_channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(),
            "com.example.media_notification_service/media",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin_pointer = plugin.get();

    method_channel->SetMethodCallHandler(
        [plugin_pointer](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    plugin->media_stream_handler_.RegisterEventChannel(
        registrar,
        "com.example.media_notification_service/media_stream",
        [plugin_pointer](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     {
                    plugin_pointer->media_session_manager_.SetupMediaEventListeners(
                        [plugin_pointer](bool song_changed)
                        {
                            plugin_pointer->OnMediaChanged(song_changed);
                        });
                    plugin_pointer->OnMediaChanged(true); });
        },
        [plugin_pointer](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     { plugin_pointer->media_session_manager_.RemoveMediaEventListeners(); });
        });

    plugin->position_stream_handler_.RegisterEventChannel(
        registrar,
        "com.example.media_notification_service/position_stream",
        [plugin_pointer](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     { plugin_pointer->media_session_manager_.SetupPositionEventListeners(
                                                           [plugin_pointer]()
                                                           {
                                                             plugin_pointer->OnPositionChanged();
                                                           }); });

          plugin_pointer->position_timer_.Start(
              std::chrono::milliseconds(100),
              [plugin_pointer]()
              {
                plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                           { plugin_pointer->OnPositionChanged(); });
              });
        },
        [plugin_pointer](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->position_timer_.Stop();
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     { plugin_pointer->media_session_manager_.RemovePositionEventListeners(); });
        });

    // queue stream is not supported on Windows
    plugin->queue_stream_handler_.RegisterEventChannel(
        registrar,
        "com.example.media_notification_service/queue_stream");

    registrar->AddPlugin(std::move(plugin));
  }

  MediaNotificationServicePlugin::MediaNotificationServicePlugin()
  {
    worker_thread_.EnqueueTask([this]()
                               { media_session_manager_.Initialize(); });
  }

  MediaNotificationServicePlugin::~MediaNotificationServicePlugin()
  {
    worker_thread_.EnqueueTask([this]()
                               { media_session_manager_.RemoveMediaEventListeners(); });
  }

  void MediaNotificationServicePlugin::OnMediaChanged(bool song_changed)
  {
    auto map = media_session_manager_.GetCurrentMediaInfo();
    map[flutter::EncodableValue("songChanged")] = flutter::EncodableValue(song_changed);
    media_stream_handler_.Send(flutter::EncodableValue(map));
  }

  void MediaNotificationServicePlugin::OnPositionChanged()
  {
    auto map = media_session_manager_.GetCurrentPositionInfo();
    position_stream_handler_.Send(flutter::EncodableValue(map));
  }

  void MediaNotificationServicePlugin::HandleMethodCall(
      const flutter::MethodCall<flutter::EncodableValue> &method_call,
      std::unique_ptr<flutter::MethodResult<flutter::EncodableValue>> result)
  {
    std::string name = method_call.method_name();
    Method method = MethodStringToEnum(name);

    switch (method)
    {
    case Method::GetCurrentMedia:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
             auto map = media_session_manager_.GetCurrentMediaInfo();
             result->Success(flutter::EncodableValue(map)); });
    }
    break;
    case Method::PlayPause:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
             auto success = media_session_manager_.PlayPause();
             result->Success(flutter::EncodableValue(success)); });
    }
    break;
    case Method::SkipToNext:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
             auto success = media_session_manager_.SkipToNext();
             result->Success(flutter::EncodableValue(success)); });
    }
    break;
    case Method::SkipToPrevious:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
             auto success = media_session_manager_.SkipToPrevious();
             result->Success(flutter::EncodableValue(success)); });
    }
    break;
    case Method::Stop:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
             auto success = media_session_manager_.Stop();
             result->Success(flutter::EncodableValue(success)); });
    }
    case Method::SeekTo:
    {
      auto result_shared = std::shared_ptr<flutter::MethodResult<flutter::EncodableValue>>(std::move(result));

      int64_t position_ms = 0;
      if (const auto *arg = std::get_if<flutter::EncodableMap>(method_call.arguments()))
      {
        auto it = arg->find(flutter::EncodableValue("position"));
        if (it != arg->end())
        {
          position_ms = it->second.LongValue();
        }
      }

      worker_thread_.EnqueueTask([this, position_ms, result = result_shared]()
                                 { 
                                   auto success = media_session_manager_.SeekTo(position_ms);
                                   result->Success(flutter::EncodableValue(success)); });
    }
    break;
    // methods not supported on Windows
    case Method::GetQueue:
    {
      flutter::EncodableList list = {};
      result->Success(flutter::EncodableValue(list));
    }
    break;
    case Method::SkipToQueueItem:
    case Method::HasPermission:
    case Method::OpenSettings:
    {
      result->Success(flutter::EncodableValue(true));
    }
    break;
    case Method::Unknown:
    default:
      result->NotImplemented();
      break;
    }
  }

  Method MediaNotificationServicePlugin::MethodStringToEnum(const std::string &method_name)
  {
    static const std::unordered_map<std::string, Method> method_map = {
        {"getCurrentMedia", Method::GetCurrentMedia},
        {"getQueue", Method::GetQueue},
        {"hasPermission", Method::HasPermission},
        {"openSettings", Method::OpenSettings},
        {"playPause", Method::PlayPause},
        {"skipToNext", Method::SkipToNext},
        {"skipToPrevious", Method::SkipToPrevious},
        {"stop", Method::Stop},
        {"seekTo", Method::SeekTo},
        {"skipToQueueItem", Method::SkipToQueueItem}};

    auto it = method_map.find(method_name);
    if (it != method_map.end())
    {
      return it->second;
    }

    return Method::Unknown;
  }
} // namespace media_notification_service