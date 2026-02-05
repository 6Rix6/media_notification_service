#include "include/media_notification_service/media_notification_service_plugin_c_api.h"

#include <flutter/plugin_registrar_windows.h>

#include "media_notification_service_plugin.h"

void MediaNotificationServicePluginCApiRegisterWithRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  media_notification_service::MediaNotificationServicePlugin::RegisterWithRegistrar(
      flutter::PluginRegistrarManager::GetInstance()
          ->GetRegistrar<flutter::PluginRegistrarWindows>(registrar));
}
