#include "media_session_manager.h"
#include <flutter/encodable_value.h>

using namespace winrt::Windows::Media::Control;

namespace media_notification_service
{
    MediaSessionManager::MediaSessionManager() = default;

    MediaSessionManager::~MediaSessionManager()
    {
        RemoveEventListeners();
    }

    bool MediaSessionManager::Initialize()
    {
        try
        {
            media_manager_ = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
            return media_manager_ != nullptr;
        }
        catch (...)
        {
            return false;
        }
    }

    flutter::EncodableMap MediaSessionManager::GetCurrentMediaInfo()
    {
        flutter::EncodableMap map;

        try
        {
            if (!media_manager_)
            {
                return map;
            }

            auto session = media_manager_.GetCurrentSession();
            if (!session)
            {
                return map;
            }

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
        catch (...)
        {
        }

        return map;
    }

    void MediaSessionManager::SetupEventListeners(MediaChangedCallback callback)
    {
        on_media_changed_ = callback;

        if (!media_manager_)
            return;

        try
        {
            sessions_changed_token_ = media_manager_.SessionsChanged(
                [this](auto &&, auto &&)
                {
                    if (on_media_changed_)
                    {
                        on_media_changed_();
                    }
                });

            auto session = media_manager_.GetCurrentSession();
            if (session)
            {
                media_properties_changed_token_ = session.MediaPropertiesChanged(
                    [this](auto &&, auto &&)
                    {
                        if (on_media_changed_)
                        {
                            on_media_changed_();
                        }
                    });

                playback_info_changed_token_ = session.PlaybackInfoChanged(
                    [this](auto &&, auto &&)
                    {
                        if (on_media_changed_)
                        {
                            on_media_changed_();
                        }
                    });
            }
        }
        catch (...)
        {
        }
    }

    void MediaSessionManager::RemoveEventListeners()
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

        on_media_changed_ = nullptr;
    }

} // namespace media_notification_service