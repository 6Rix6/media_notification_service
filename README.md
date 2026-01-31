# Media Notification Service

A Flutter plugin for Android that allows you to access and control media playing from other apps through the Android MediaSession API.

## Features

- Access currently playing media information (title, artist, album, album art)
- Control playback (play, pause, skip, stop, seek)
- Real-time playback position tracking
- Queue information (next/previous tracks)
- Stream-based updates for media changes
- Album art retrieval

## Requirements

- Flutter SDK: `>=3.0.0`
- Android SDK: API 21+ (Android 5.0 Lollipop or higher)
- Kotlin support enabled in your Android project

## Setup

### 1. Update AndroidManifest.xml

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

### 2. Request Notification Listener Permission

Users must grant notification listener permission to your app. You can check and request this permission:

```dart
final service = MediaNotificationService();

// Check if permission is granted
final hasPermission = await service.hasPermission();

if (!hasPermission) {
  // Open system settings to grant permission
  await service.openSettings();
}
```

## Usage

### Basic Usage

```dart
import 'package:media_notification_service/media_notification_service.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatefulWidget {
  @override
  _MyAppState createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final _service = MediaNotificationService();
  MediaInfo? _currentMedia;
  PositionInfo? _position;

  @override
  void initState() {
    super.initState();
    _initService();
  }

  void _initService() {
    // Listen to media changes
    _service.mediaStream.listen((data) {
      if (data != null) {
        setState(() {
          _currentMedia = data.mediaInfo;
        });
      }
    });

    // Listen to position updates
    _service.positionStream.listen((position) {
      setState(() {
        _position = position;
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: Text('Media Controller')),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              if (_currentMedia?.albumArt != null)
                Image.memory(_currentMedia!.albumArt!, height: 200),
              SizedBox(height: 20),
              Text(_currentMedia?.title ?? 'No media'),
              Text(_currentMedia?.artist ?? ''),
              SizedBox(height: 20),
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  IconButton(
                    icon: Icon(Icons.skip_previous),
                    onPressed: () => _service.skipToPrevious(),
                  ),
                  IconButton(
                    icon: Icon(_currentMedia?.isPlaying == true
                        ? Icons.pause
                        : Icons.play_arrow),
                    onPressed: () => _service.playPause(),
                  ),
                  IconButton(
                    icon: Icon(Icons.skip_next),
                    onPressed: () => _service.skipToNext(),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }
}
```

## API Reference

### MediaNotificationService

Main service class for interacting with media sessions.

#### Methods

| Method              | Return Type                   | Description                                               |
| ------------------- | ----------------------------- | --------------------------------------------------------- |
| `mediaStream`       | `Stream<MediaInfoWithQueue?>` | Stream of media information updates                       |
| `positionStream`    | `Stream<PositionInfo?>`       | Stream of playback position updates                       |
| `getCurrentMedia()` | `Future<MediaInfo?>`          | Get current media information                             |
| `hasPermission()`   | `Future<bool>`                | Check if notification listener permission is granted      |
| `openSettings()`    | `Future<void>`                | Open system settings for notification listener permission |
| `playPause()`       | `Future<bool>`                | Toggle play/pause                                         |
| `skipToNext()`      | `Future<bool>`                | Skip to next track                                        |
| `skipToPrevious()`  | `Future<bool>`                | Skip to previous track                                    |
| `stop()`            | `Future<bool>`                | Stop playback                                             |
| `seekTo(Duration)`  | `Future<bool>`                | Seek to specific position                                 |

### Models

#### MediaInfo

```dart
class MediaInfo {
  final String? title;
  final String? artist;
  final String? album;
  final String? packageName;
  final Uint8List? albumArt;
  final bool isPlaying;
  final PlaybackState state;
}
```

#### QueueItem

```dart
class QueueItem {
  final String? title;
  final String? artist;
  final Uint8List? albumArt;
  final Uri? albumArtUri;
}
```

#### PositionInfo

```dart
class PositionInfo {
  final Duration position;
  final Duration duration;
  final double playbackSpeed;
  final PlaybackState state;

  double get progress; // Returns 0.0 to 1.0
}
```

#### MediaInfoWithQueue

```dart
class MediaInfoWithQueue {
  final MediaInfo mediaInfo;
  final QueueItem? nextItem;
  final QueueItem? previousItem;
  final bool songChanged;
  final bool queueChanged;
}
```

#### PlaybackState

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

see [Android Developers](https://developer.android.com/reference/android/media/session/PlaybackState#nested-classes)

## Important Notes

### Permissions

- Users must manually grant Notification Listener permission in system settings
- The app cannot programmatically grant this permission
- Always check `hasPermission()` before using media controls

### Album Art Optimization

To prevent unnecessary memory usage, album art is only updated when:

- The song changes
- Album art was previously null
- You explicitly request it via `getCurrentMedia()`

### Limitations

- Only works on Android
- Requires the media app to properly implement MediaSession
- Some apps may not provide complete metadata
- Album art size depends on the source app (may be large)

## TODO

- [ ] Add proper error handling
