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
        using EventListenerCallback = std::function<void()>;

        MediaSessionManager();
        ~MediaSessionManager();

        MediaSessionManager(const MediaSessionManager &) = delete;
        MediaSessionManager &operator=(const MediaSessionManager &) = delete;

        bool Initialize();

        flutter::EncodableMap GetCurrentMediaInfo();
        flutter::EncodableMap GetCurrentPositionInfo();

        void SetupMediaEventListeners(EventListenerCallback callback);
        void RemoveMediaEventListeners();

        void SetupPositionEventListeners(EventListenerCallback callback);
        void RemovePositionEventListeners();

        bool IsPlaying();

        void callCallbacks();

        bool PlayPause();
        bool SkipToNext();
        bool SkipToPrevious();
        bool Stop();
        bool SeekTo(int64_t position_ms);

    private:
        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager media_manager_{nullptr};

        // tokens for media change event
        winrt::event_token sessions_changed_token_;
        winrt::event_token current_session_changed_token_;
        winrt::event_token media_properties_changed_token_;
        winrt::event_token playback_info_changed_token_;

        // tokens for position change event
        winrt::event_token timeline_properties_changed_token_;

        // callbacks
        EventListenerCallback on_media_changed_;
        EventListenerCallback on_position_changed_;

        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession GetCurrentSession();

        std::string PlaybackStatusToString(
            winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus status);

        std::vector<uint8_t> IRandomAccessStreamReferenceToByteArray(
            winrt::Windows::Storage::Streams::IRandomAccessStreamReference const &stream_ref);
    };

} // namespace media_notification_service

#endif // MEDIA_SESSION_MANAGER_H_