#include "media_notification_service_plugin.h"

// This must be included before many other Windows headers.
#include <windows.h>

// For getPlatformVersion; remove unless needed for your plugin implementation.
#include <VersionHelpers.h>

#include <flutter/method_channel.h>
#include <flutter/plugin_registrar_windows.h>
#include <flutter/standard_method_codec.h>

#include <winrt/windows.foundation.h>
#include <winrt/Windows.Media.Control.h>

#include <memory>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

using namespace winrt::Windows::Media::Control;

namespace media_notification_service
{

  // static
  void MediaNotificationServicePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarWindows *registrar)
  {
    auto channel =
        std::make_unique<flutter::MethodChannel<flutter::EncodableValue>>(
            registrar->messenger(), "com.example.media_notification_service/media",
            &flutter::StandardMethodCodec::GetInstance());

    auto plugin = std::make_unique<MediaNotificationServicePlugin>();

    channel->SetMethodCallHandler(
        [plugin_pointer = plugin.get()](const auto &call, auto result)
        {
          plugin_pointer->HandleMethodCall(call, std::move(result));
        });

    registrar->AddPlugin(std::move(plugin));
  }

  MediaNotificationServicePlugin::MediaNotificationServicePlugin()
      : stop_worker_(false)
  {
    worker_thread_ = std::thread(&MediaNotificationServicePlugin::WorkerThreadFunc, this);
  }

  MediaNotificationServicePlugin::~MediaNotificationServicePlugin()
  {
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
            }
          }
        }
        catch (...)
        {
        }

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