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

    method_channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    plugin->media_stream_handler_.RegisterEventChannel(
        registrar,
        "com.example.media_notification_service/media_stream",
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     {
                    plugin_pointer->media_session_manager_.SetupMediaEventListeners(
                        [plugin_pointer]()
                        {
                            plugin_pointer->OnMediaChanged();
                        });
                    plugin_pointer->OnMediaChanged(); });
        },
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     { plugin_pointer->media_session_manager_.RemoveMediaEventListeners(); });
        });

    plugin->position_stream_handler_.RegisterEventChannel(
        registrar,
        "com.example.media_notification_service/position_stream",
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments)
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
        [plugin_pointer = plugin.get()](const flutter::EncodableValue *arguments)
        {
          plugin_pointer->position_timer_.Stop();
          plugin_pointer->worker_thread_.EnqueueTask([plugin_pointer]()
                                                     { plugin_pointer->media_session_manager_.RemovePositionEventListeners(); });
        });

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

  void MediaNotificationServicePlugin::OnMediaChanged()
  {
    auto map = media_session_manager_.GetCurrentMediaInfo();
    media_stream_handler_.Send(flutter::EncodableValue(map));
  }

  void MediaNotificationServicePlugin::OnPositionChanged()
  {
    if (media_session_manager_.IsPlaying())
    {
      auto map = media_session_manager_.GetCurrentPositionInfo();
      position_stream_handler_.Send(flutter::EncodableValue(map));
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

      worker_thread_.EnqueueTask([this, result = result_shared]()
                                 {
                auto map = media_session_manager_.GetCurrentMediaInfo();
                result->Success(flutter::EncodableValue(map)); });
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