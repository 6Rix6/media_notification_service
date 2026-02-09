# Media Notification Service

A Flutter plugin for **Android** and **Windows** that allows you to access and control media playing from other apps through the system media APIs.

## Supported Platforms

| Platform | Status |
|----------|--------|
| Android  | ✅ Fully supported |
| Windows  | ✅ Supported (with some limitations) |

## Features

- Access currently playing media information (title, artist, album, album art)
- Control playback (play, pause, skip, stop, seek)
- Real-time playback position tracking
- Queue information (Android only)
- Skip to queue item (Android only)
- Stream-based updates for media changes
- Album art retrieval

## Requirements

### Android
- Flutter SDK: `>=3.0.0`
- Kotlin support enabled in your Android project

### Windows
- Flutter SDK: `>=3.0.0`
- Windows 10 or later

## Setup

### Android

#### 1. Update AndroidManifest.xml

Add the notification listener service to your app's `AndroidManifest.xml`:

```xml
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.yourapp">

    <uses-permission android:name="android.permission.BIND_NOTIFICATION_LISTENER_SERVICE" />

    <application
        android:label="your_app"
        android:icon="@mipmap/ic_launcher">

        <!-- Add this service -->
        <service
            android:name="com.example.media_notification_service.MediaNotificationListener"
            android:label="Media Notification Listener"
            android:permission="android.permission.BIND_NOTIFICATION_LISTENER_SERVICE"
            android:exported="true">
            <intent-filter>
                <action android:name="android.service.notification.NotificationListenerService" />
            </intent-filter>
        </service>

        <activity
            android:name=".MainActivity"
            ...>
        </activity>
    </application>
</manifest>
```

#### 2. Request Notification Listener Permission

Users must grant notification listener permission to your app. You can check and request this permission:

```dart
final service = MediaNotificationService();

// Check if permission is granted
bool hasPermission = await service.hasPermission();

if (!hasPermission) {
  // Open system settings to grant permission
  // openSettings() waits for the user to return from settings
  await service.openSettings();

  // Check again after opening settings
  hasPermission = await service.hasPermission();
}
```

### Windows

No special setup required. Windows uses the System Media Transport Controls (SMTC) API which doesn't require any permissions.

## API Reference

> see example app for more details.

### MediaNotificationService

Main service class for interacting with media sessions.

#### Methods

| Method                      | Return Type                   | Description                                               | Android | Windows |
| --------------------------- | ----------------------------- | --------------------------------------------------------- | :-----: | :-----: |
| `mediaStream`               | `Stream<MediaInfoWithQueue?>` | Stream of media information updates                       | ✅ | ✅ |
| `positionStream`            | `Stream<PositionInfo?>`       | Stream of playback position updates                       | ✅ | ✅ |
| `queueStream`               | `Stream<List<QueueItem?>?>`   | Stream of queue updates                                   | ✅ | ❌ |
| `getCurrentMedia()`         | `Future<MediaInfo?>`          | Get current media information                             | ✅ | ✅ |
| `getQueue()`                | `Future<List<QueueItem?>?>`   | Get current queue                                         | ✅ | ❌ |
| `hasPermission()`           | `Future<bool>`                | Check if notification listener permission is granted      | ✅ | ⚪ |
| `openSettings()`            | `Future<void>`                | Open system settings for notification listener permission | ✅ | ⚪ |
| `playPause()`               | `Future<bool>`                | Toggle play/pause                                         | ✅ | ✅ |
| `skipToNext()`              | `Future<bool>`                | Skip to next track                                        | ✅ | ✅ |
| `skipToPrevious()`          | `Future<bool>`                | Skip to previous track                                    | ✅ | ✅ |
| `stop()`                    | `Future<bool>`                | Stop playback                                             | ✅ | ✅ |
| `seekTo(Duration position)` | `Future<bool>`                | Seek to specific position                                 | ✅ | ✅ |
| `skipToQueueItem(int id)`   | `Future<bool>`                | Skip to specific queue item                               | ✅ | ❌ |

> **Legend**: ✅ Supported | ❌ Not supported (returns empty/false) | ⚪ Not applicable (always returns true)

### PlaybackState

Enum representing the current playback state:

- `none`
- `stopped`
- `paused`
- `playing`
- `fastForwarding`
- `rewinding`
- `buffering`
- `error`
- `connecting`
- `skippingToPrevious`
- `skippingToNext`
- `skippingToQueueItem`

Based on [Android PlaybackState](https://developer.android.com/reference/android/media/session/PlaybackState#nested-classes)

## Important Notes

### Permissions

- Users must manually grant Notification Listener permission in system settings
- The app cannot programmatically grant this permission
- Always check `hasPermission()` before using media controls

### Limitations

#### Android
- Requires the media app to properly implement MediaSession
- Some apps may not provide complete metadata
- Album art size depends on the source app (may be large)

#### Windows
- Queue-related features are not supported (`queueStream`, `getQueue()`, `skipToQueueItem()`)
- Permission methods (`hasPermission()`, `openSettings()`) always return `true` / do nothing as no permission is required
- Requires the media app to use Windows System Media Transport Controls (SMTC)
- `positionStream` stability depends on the media app's SMTC implementation
  - ✅ Works correctly: Spotify (desktop app)
  - ⚠️ Unstable: YouTube Music (browser version)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
