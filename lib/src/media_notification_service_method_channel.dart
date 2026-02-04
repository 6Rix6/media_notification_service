import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'models.dart';
import 'media_notification_service_platform_interface.dart';

class MediaNotificationServiceMethodChannel
    extends MediaNotificationServicePlatform {
  @visibleForTesting
  static const methodChannel = MethodChannel(
    'com.example.media_notification_service/media',
  );

  @visibleForTesting
  static const mediaEventChannel = EventChannel(
    'com.example.media_notification_service/media_stream',
  );

  @visibleForTesting
  static const positionEventChannel = EventChannel(
    'com.example.media_notification_service/position_stream',
  );

  @visibleForTesting
  static const queueEventChannel = EventChannel(
    'com.example.media_notification_service/queue_stream',
  );

  Stream<MediaInfoWithQueue?>? _mediaStream;
  Stream<PositionInfo?>? _positionStream;
  Stream<List<QueueItem?>>? _queueStream;

  @override
  Stream<MediaInfoWithQueue?> get mediaStream {
    _mediaStream ??= mediaEventChannel.receiveBroadcastStream().map((event) {
      if (event == null) return null;
      final map = event as Map;
      return MediaInfoWithQueue.fromMap(map);
    });
    return _mediaStream!;
  }

  @override
  Stream<PositionInfo?> get positionStream {
    _positionStream ??= positionEventChannel.receiveBroadcastStream().map((
      event,
    ) {
      if (event == null) return null;
      final map = event as Map;
      return PositionInfo.fromMap(map);
    });
    return _positionStream!;
  }

  @override
  Stream<List<QueueItem?>> get queueStream {
    _queueStream ??= queueEventChannel.receiveBroadcastStream().map((event) {
      if (event == null) return [];
      final list = event as List;
      return list.map((e) => QueueItem.fromMap(e)).toList();
    });
    return _queueStream!;
  }

  @override
  Future<MediaInfo?> getCurrentMedia() async {
    try {
      final Map<dynamic, dynamic>? result = await methodChannel.invokeMethod(
        'getCurrentMedia',
      );
      if (result == null) return null;
      return MediaInfo.fromMap(result);
    } on PlatformException catch (e) {
      print("Failed to get media info: ${e.message}");
      return null;
    }
  }

  @override
  Future<List<QueueItem?>> getQueue() async {
    try {
      final List<dynamic>? result = await methodChannel.invokeMethod(
        'getQueue',
      );
      if (result == null) return [];
      return result.map((e) => QueueItem.fromMap(e)).toList();
    } on PlatformException catch (e) {
      print("Failed to get queue: ${e.message}");
      return [];
    }
  }

  @override
  Future<bool> hasPermission() async {
    try {
      final bool result = await methodChannel.invokeMethod('hasPermission');
      return result;
    } catch (e) {
      return false;
    }
  }

  @override
  Future<void> openSettings() async {
    try {
      await methodChannel.invokeMethod('openSettings');
    } catch (e) {
      print("Failed to open settings: $e");
    }
  }

  @override
  Future<bool> playPause() async {
    try {
      final bool result = await methodChannel.invokeMethod('playPause');
      return result;
    } catch (e) {
      print("Failed to play/pause: $e");
      return false;
    }
  }

  @override
  Future<bool> skipToNext() async {
    try {
      final bool result = await methodChannel.invokeMethod('skipToNext');
      return result;
    } catch (e) {
      print("Failed to skip to next: $e");
      return false;
    }
  }

  @override
  Future<bool> skipToPrevious() async {
    try {
      final bool result = await methodChannel.invokeMethod('skipToPrevious');
      return result;
    } catch (e) {
      print("Failed to skip to previous: $e");
      return false;
    }
  }

  @override
  Future<bool> stop() async {
    try {
      final bool result = await methodChannel.invokeMethod('stop');
      return result;
    } catch (e) {
      print("Failed to stop: $e");
      return false;
    }
  }

  @override
  Future<bool> seekTo(Duration position) async {
    try {
      final bool result = await methodChannel.invokeMethod('seekTo', {
        'position': position.inMilliseconds,
      });
      return result;
    } catch (e) {
      print("Failed to seek: $e");
      return false;
    }
  }

  @override
  Future<bool> skipToQueueItem(int id) async {
    try {
      final bool result = await methodChannel.invokeMethod('skipToQueueItem', {
        'id': id,
      });
      return result;
    } catch (e) {
      print("Failed to skip to queue item: $e");
      return false;
    }
  }
}
