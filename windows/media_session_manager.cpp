#include "media_session_manager.h"
#include <flutter/encodable_value.h>
#include <flutter/method_channel.h>
#include <flutter/standard_method_codec.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
using namespace Windows::Foundation;

namespace media_notification_service
{
    MediaSessionManager::MediaSessionManager() = default;

    MediaSessionManager::~MediaSessionManager()
    {
        RemoveMediaEventListeners();
        RemovePositionEventListeners();
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
            auto playback_info = session.GetPlaybackInfo();
            auto status = playback_info.PlaybackStatus();
            auto playback_state = PlaybackStatusToString(status);
            bool is_playing = (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);

            auto thumbnail = props.Thumbnail();
            if (thumbnail)
            {
                auto image_data = IRandomAccessStreamReferenceToByteArray(thumbnail);
                map[flutter::EncodableValue("albumArt")] =
                    flutter::EncodableValue(flutter::EncodableValue(image_data));
            }

            map[flutter::EncodableValue("title")] =
                flutter::EncodableValue(winrt::to_string(props.Title()));
            map[flutter::EncodableValue("artist")] =
                flutter::EncodableValue(winrt::to_string(props.Artist()));
            map[flutter::EncodableValue("album")] =
                flutter::EncodableValue(winrt::to_string(props.AlbumTitle()));
            map[flutter::EncodableValue("state")] =
                flutter::EncodableValue(playback_state);
            map[flutter::EncodableValue("isPlaying")] = flutter::EncodableValue(is_playing);
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
            auto playback_rate = playback_info.PlaybackRate().Value();

            if (playback_rate <= 0.0)
            {
                playback_rate = 1.0;
            }

            int64_t stored_position = timeline.Position().count() / 10000;
            int64_t duration = timeline.EndTime().count() / 10000;

            auto last_updated = timeline.LastUpdatedTime();
            auto last_updated_ms = winrt::clock::to_time_t(last_updated) * 1000;

            auto now = winrt::clock::now();
            auto now_ms = winrt::clock::to_time_t(now) * 1000;

            int64_t current_position = stored_position;

            if (status == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing)
            {
                int64_t elapsed = now_ms - last_updated_ms;
                int64_t adjusted_elapsed = static_cast<int64_t>(elapsed * playback_rate);
                current_position = stored_position + adjusted_elapsed;

                if (duration > 0 && current_position > duration)
                {
                    current_position = duration;
                }
            }

            auto playback_state = PlaybackStatusToString(status);

            map[flutter::EncodableValue("position")] = flutter::EncodableValue(current_position);
            map[flutter::EncodableValue("duration")] = flutter::EncodableValue(duration);
            map[flutter::EncodableValue("state")] = flutter::EncodableValue(playback_state);
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

    void MediaSessionManager::callCallbacks()
    {
        if (on_media_changed_)
        {
            on_media_changed_(true);
        }
        if (on_position_changed_)
        {
            on_position_changed_();
        }
    }

    bool MediaSessionManager::PlayPause()
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            session.TryTogglePlayPauseAsync().get();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaSessionManager::SkipToNext()
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            session.TrySkipNextAsync().get();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaSessionManager::SkipToPrevious()
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            session.TrySkipPreviousAsync().get();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaSessionManager::Stop()
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            session.TryStopAsync().get();
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool MediaSessionManager::SeekTo(int64_t position_ms)
    {
        try
        {
            auto session = GetCurrentSession();
            if (!session)
            {
                return false;
            }

            int64_t ticks = position_ms * 10000;
            bool success = session.TryChangePlaybackPositionAsync(ticks).get();

            return success;
        }
        catch (...)
        {
            return false;
        }
    }

    std::string MediaSessionManager::PlaybackStatusToString(
        GlobalSystemMediaTransportControlsSessionPlaybackStatus status)
    {
        switch (status)
        {
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing:
            return "STATE_PLAYING";
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused:
            return "STATE_PAUSED";
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Stopped:
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Closed:
            return "STATE_STOPPED";
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Changing:
        case GlobalSystemMediaTransportControlsSessionPlaybackStatus::Opened:
            return "STATE_BUFFERING";
        default:
            return "STATE_NONE";
            break;
        }
    }

    std::vector<uint8_t> MediaSessionManager::IRandomAccessStreamReferenceToByteArray(
        winrt::Windows::Storage::Streams::IRandomAccessStreamReference const &stream_ref)
    {
        auto thumbnailStream = stream_ref.OpenReadAsync().get();
        uint64_t size = thumbnailStream.Size();

        Buffer buffer(static_cast<uint32_t>(size));
        thumbnailStream.ReadAsync(buffer, static_cast<uint32_t>(size), InputStreamOptions::None).get();

        std::vector<uint8_t> bytes(size);
        auto dataReader = DataReader::FromBuffer(buffer);
        dataReader.ReadBytes(bytes);

        return bytes;
    }

    // event listeners
    void MediaSessionManager::SetupSessionSpecificListeners()
    {
        auto session = GetCurrentSession();
        if (!session)
            return;

        try
        {
            media_properties_changed_token_ = session.MediaPropertiesChanged(
                [this](auto &&, auto &&)
                {
                    if (on_media_changed_)
                    {
                        on_media_changed_(true);
                    }
                });

            playback_info_changed_token_ = session.PlaybackInfoChanged(
                [this](auto &&, auto &&)
                {
                    if (on_media_changed_)
                    {
                        on_media_changed_(false);
                    }
                });
        }
        catch (...)
        {
        }
    }

    void MediaSessionManager::RemoveSessionSpecificListeners()
    {
        if (!media_manager_)
            return;

        try
        {
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

    void MediaSessionManager::SetupPositionSessionSpecificListeners()
    {
        auto session = GetCurrentSession();
        if (!session)
            return;

        try
        {
            timeline_properties_changed_token_ = session.TimelinePropertiesChanged(
                [this](auto &&, auto &&)
                {
                    if (on_position_changed_)
                    {
                        on_position_changed_();
                    }
                });

            playback_info_changed_token_for_position_ = session.PlaybackInfoChanged(
                [this](auto &&, auto &&)
                {
                    if (on_position_changed_)
                    {
                        on_position_changed_();
                    }
                });

            media_properties_changed_token_for_position_ = session.MediaPropertiesChanged(
                [this](auto &&, auto &&)
                {
                    if (on_position_changed_)
                    {
                        on_position_changed_();
                    }
                });
        }
        catch (...)
        {
        }
    }

    void MediaSessionManager::RemovePositionSessionSpecificListeners()
    {
        if (!media_manager_)
            return;

        try
        {
            auto session = media_manager_.GetCurrentSession();
            if (session)
            {
                if (timeline_properties_changed_token_)
                {
                    session.TimelinePropertiesChanged(timeline_properties_changed_token_);
                    timeline_properties_changed_token_ = {};
                }

                if (playback_info_changed_token_for_position_)
                {
                    session.PlaybackInfoChanged(playback_info_changed_token_for_position_);
                    playback_info_changed_token_for_position_ = {};
                }

                if (media_properties_changed_token_for_position_)
                {
                    session.MediaPropertiesChanged(media_properties_changed_token_for_position_);
                    media_properties_changed_token_for_position_ = {};
                }
            }
        }
        catch (...)
        {
        }
    }

    void MediaSessionManager::SetupMediaEventListeners(MediaEventListenerCallback callback)
    {
        on_media_changed_ = callback;

        if (!media_manager_)
            return;

        try
        {
            sessions_changed_token_ = media_manager_.SessionsChanged(
                [this](auto &&, auto &&)
                {
                    callCallbacks();
                });

            current_session_changed_token_ = media_manager_.CurrentSessionChanged(
                [this](auto &&, auto &&)
                {
                    RemoveSessionSpecificListeners();
                    SetupSessionSpecificListeners();
                    callCallbacks();
                });

            SetupSessionSpecificListeners();
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
            current_session_changed_token_for_position_ = media_manager_.CurrentSessionChanged(
                [this](auto &&, auto &&)
                {
                    RemovePositionSessionSpecificListeners();
                    SetupPositionSessionSpecificListeners();
                    if (on_position_changed_)
                    {
                        on_position_changed_();
                    }
                });

            SetupPositionSessionSpecificListeners();
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
            if (current_session_changed_token_for_position_)
            {
                media_manager_.CurrentSessionChanged(current_session_changed_token_for_position_);
                current_session_changed_token_for_position_ = {};
            }

            RemovePositionSessionSpecificListeners();
        }
        catch (...)
        {
        }

        on_position_changed_ = nullptr;
    }

} // namespace media_notification_service