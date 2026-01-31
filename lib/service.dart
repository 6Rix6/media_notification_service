import 'package:flutter/services.dart';
import 'models.dart';

class MediaNotificationService {
  static const _platform = MethodChannel(
    'com.example.media_notification_service/media',
  );
  static const _eventChannel = EventChannel(
    'com.example.media_notification_service/media_stream',
  );
  static const _positionEventChannel = EventChannel(
    'com.example.media_notification_service/position_stream',
  );

  Stream<MediaInfoWithQueue?>? _mediaStream;
  Stream<PositionInfo?>? _positionStream;

  Stream<MediaInfoWithQueue?> get mediaStream {
    _mediaStream ??= _eventChannel.receiveBroadcastStream().map((event) {
      if (event == null) return null;
      final map = event as Map;
      return MediaInfoWithQueue.fromMap(map);
    });
    return _mediaStream!;
  }

  Stream<PositionInfo?> get positionStream {
    _positionStream ??= _positionEventChannel.receiveBroadcastStream().map((
      event,
    ) {
      if (event == null) return null;
      final map = event as Map;
      return PositionInfo.fromMap(map);
    });
    return _positionStream!;
  }

  Future<MediaInfo?> getCurrentMedia() async {
    try {
      final Map<dynamic, dynamic>? result = await _platform.invokeMethod(
        'getCurrentMedia',
      );
      if (result == null) return null;
      return MediaInfo.fromMap(result);
    } on PlatformException catch (e) {
      print("Failed to get media info: ${e.message}");
      return null;
    }
  }

  Future<bool> hasPermission() async {
    try {
      final bool result = await _platform.invokeMethod('hasPermission');
      return result;
    } catch (e) {
      return false;
    }
  }

  Future<void> openSettings() async {
    try {
      await _platform.invokeMethod('openSettings');
    } catch (e) {
      print("Failed to open settings: $e");
    }
  }

  Future<bool> playPause() async {
    try {
      final bool result = await _platform.invokeMethod('playPause');
      return result;
    } catch (e) {
      print("Failed to play/pause: $e");
      return false;
    }
  }

  Future<bool> skipToNext() async {
    try {
      final bool result = await _platform.invokeMethod('skipToNext');
      return result;
    } catch (e) {
      print("Failed to skip to next: $e");
      return false;
    }
  }

  Future<bool> skipToPrevious() async {
    try {
      final bool result = await _platform.invokeMethod('skipToPrevious');
      return result;
    } catch (e) {
      print("Failed to skip to previous: $e");
      return false;
    }
  }

  Future<bool> stop() async {
    try {
      final bool result = await _platform.invokeMethod('stop');
      return result;
    } catch (e) {
      print("Failed to stop: $e");
      return false;
    }
  }

  Future<bool> seekTo(Duration position) async {
    try {
      final bool result = await _platform.invokeMethod('seekTo', {
        'position': position.inMilliseconds,
      });
      return result;
    } catch (e) {
      print("Failed to seek: $e");
      return false;
    }
  }
}
