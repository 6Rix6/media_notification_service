#ifndef MEDIA_SESSION_MANAGER_H_
#define MEDIA_SESSION_MANAGER_H_

#include <flutter/encodable_value.h>

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Foundation.h>
#include <functional>

namespace media_notification_service
{
    class MediaSessionManager
    {
    public:
        using MediaChangedCallback = std::function<void()>;

        MediaSessionManager();
        ~MediaSessionManager();

        MediaSessionManager(const MediaSessionManager &) = delete;
        MediaSessionManager &operator=(const MediaSessionManager &) = delete;

        bool Initialize();

        flutter::EncodableMap GetCurrentMediaInfo();

        void SetupEventListeners(MediaChangedCallback callback);
        void RemoveEventListeners();

    private:
        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager media_manager_{nullptr};

        winrt::event_token sessions_changed_token_;
        winrt::event_token media_properties_changed_token_;
        winrt::event_token playback_info_changed_token_;

        MediaChangedCallback on_media_changed_;
    };

} // namespace media_notification_service

#endif // MEDIA_SESSION_MANAGER_H_