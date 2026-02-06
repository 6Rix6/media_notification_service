#include "media_session_manager.h"
#include <flutter/encodable_value.h>

using namespace winrt::Windows::Media::Control;

namespace media_notification_service
{
    MediaSessionManager::MediaSessionManager() = default;

    MediaSessionManager::~MediaSessionManager()
    {
        RemoveMediaEventListeners();
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

    // helpers

    GlobalSystemMediaTransportControlsSession MediaSessionManager::GetCurrentSession()
    {
        if (!media_manager_)
        {
            return nullptr;
        }

        try
        {
            return media_manager_.GetCurrentSession();
        }
        catch (...)
        {
            return nullptr;
        }
    }

    flutter::EncodableMap MediaSessionManager::GetCurrentMediaInfo()
    {
        flutter::EncodableMap map;

        try
        {
            auto session = GetCurrentSession();

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

    flutter::EncodableMap MediaSessionManager::GetCurrentPositionInfo()
    {
        flutter::EncodableMap map;

        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return map;
            }

            auto timeline = session.GetTimelineProperties();
            auto playback_info = session.GetPlaybackInfo();
            auto status = playback_info.PlaybackStatus();

            int64_t stored_position = timeline.Position().count() / 10000;
            int64_t duration = timeline.EndTime().count() / 10000;
            int64_t start_time = timeline.StartTime().count() / 10000;

            auto last_updated = timeline.LastUpdatedTime();
            auto last_updated_ms = winrt::clock::to_time_t(last_updated) * 1000;

            auto now = winrt::clock::now();
            auto now_ms = winrt::clock::to_time_t(now) * 1000;

            int64_t current_position = stored_position;

            if (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
            {
                int64_t elapsed = now_ms - last_updated_ms;
                current_position = stored_position + elapsed;

                if (duration > 0 && current_position > duration)
                {
                    current_position = duration;
                }
            }

            map[flutter::EncodableValue("position")] = flutter::EncodableValue(current_position);
            map[flutter::EncodableValue("duration")] = flutter::EncodableValue(duration);
            map[flutter::EncodableValue("startTime")] = flutter::EncodableValue(start_time);
            map[flutter::EncodableValue("lastUpdatedTime")] = flutter::EncodableValue(last_updated_ms);

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
            map[flutter::EncodableValue("playbackState")] = flutter::EncodableValue(playback_state);
        }
        catch (...)
        {
        }

        return map;
    }

    bool MediaSessionManager::IsPlaying()
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            auto playback_info = session.GetPlaybackInfo();
            auto status = playback_info.PlaybackStatus();

            return status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        }
        catch (...)
        {
            return false;
        }
    }

    // event listeners

    void MediaSessionManager::SetupMediaEventListeners(EventListenerCallback callback)
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

            auto session = GetCurrentSession();
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

    void MediaSessionManager::RemoveMediaEventListeners()
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

    void MediaSessionManager::SetupPositionEventListeners(EventListenerCallback callback)
    {
        on_position_changed_ = callback;

        if (!media_manager_)
            return;

        try
        {
            auto session = GetCurrentSession();
            if (session)
            {
                timeline_properties_changed_token_ = session.TimelinePropertiesChanged(
                    [this](auto &&, auto &&)
                    {
                        if (on_position_changed_)
                        {
                            on_position_changed_();
                        }
                    });

                playback_info_changed_token_ = session.PlaybackInfoChanged(
                    [this](auto &&, auto &&)
                    {
                        if (on_position_changed_)
                        {
                            on_position_changed_();
                        }
                    });

                media_properties_changed_token_ = session.MediaPropertiesChanged(
                    [this](auto &&, auto &&)
                    {
                        if (on_position_changed_)
                        {
                            on_position_changed_();
                        }
                    });
            }
        }
        catch (...)
        {
        }
    }

    void MediaSessionManager::RemovePositionEventListeners()
    {
        if (!media_manager_)
            return;

        try
        {
            auto session = GetCurrentSession();
            if (session)
            {
                if (timeline_properties_changed_token_)
                {
                    session.TimelinePropertiesChanged(timeline_properties_changed_token_);
                    timeline_properties_changed_token_ = {};
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

        on_position_changed_ = nullptr;
    }
} // namespace media_notification_service