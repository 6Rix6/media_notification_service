library;

import 'src/models.dart';
import 'src/media_notification_service_platform_interface.dart';

export 'src/models.dart';
export 'src/media_notification_service_platform_interface.dart';

class MediaNotificationService {
  Stream<MediaInfoWithQueue?> get mediaStream =>
      MediaNotificationServicePlatform.instance.mediaStream;

  Stream<PositionInfo?> get positionStream =>
      MediaNotificationServicePlatform.instance.positionStream;

  Stream<List<QueueItem?>> get queueStream =>
      MediaNotificationServicePlatform.instance.queueStream;

  Future<MediaInfo?> getCurrentMedia() =>
      MediaNotificationServicePlatform.instance.getCurrentMedia();

  Future<List<QueueItem?>> getQueue() =>
      MediaNotificationServicePlatform.instance.getQueue();

  Future<bool> hasPermission() =>
      MediaNotificationServicePlatform.instance.hasPermission();

  Future<void> openSettings() =>
      MediaNotificationServicePlatform.instance.openSettings();

  Future<bool> playPause() =>
      MediaNotificationServicePlatform.instance.playPause();

  Future<bool> skipToNext() =>
      MediaNotificationServicePlatform.instance.skipToNext();

  Future<bool> skipToPrevious() =>
      MediaNotificationServicePlatform.instance.skipToPrevious();

  Future<bool> stop() => MediaNotificationServicePlatform.instance.stop();

  Future<bool> seekTo(Duration position) =>
      MediaNotificationServicePlatform.instance.seekTo(position);

  Future<bool> skipToQueueItem(int id) =>
      MediaNotificationServicePlatform.instance.skipToQueueItem(id);
}
