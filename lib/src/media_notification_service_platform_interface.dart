import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'models.dart';
import 'media_notification_service_method_channel.dart';

abstract class MediaNotificationServicePlatform extends PlatformInterface {
  MediaNotificationServicePlatform() : super(token: _token);

  static final Object _token = Object();

  static MediaNotificationServicePlatform _instance =
      MediaNotificationServiceMethodChannel();

  static MediaNotificationServicePlatform get instance => _instance;

  static set instance(MediaNotificationServicePlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  // streams
  Stream<MediaInfoWithQueue?> get mediaStream {
    throw UnimplementedError('mediaStream has not been implemented.');
  }

  Stream<PositionInfo?> get positionStream {
    throw UnimplementedError('positionStream has not been implemented.');
  }

  Stream<List<QueueItem?>> get queueStream {
    throw UnimplementedError('queueStream has not been implemented.');
  }

  // methods
  Future<MediaInfo?> getCurrentMedia() {
    throw UnimplementedError('getCurrentMedia() has not been implemented.');
  }

  Future<List<QueueItem?>> getQueue() {
    throw UnimplementedError('getQueue() has not been implemented.');
  }

  Future<bool> hasPermission() {
    throw UnimplementedError('hasPermission() has not been implemented.');
  }

  Future<void> openSettings() {
    throw UnimplementedError('openSettings() has not been implemented.');
  }

  Future<bool> playPause() {
    throw UnimplementedError('playPause() has not been implemented.');
  }

  Future<bool> skipToNext() {
    throw UnimplementedError('skipToNext() has not been implemented.');
  }

  Future<bool> skipToPrevious() {
    throw UnimplementedError('skipToPrevious() has not been implemented.');
  }

  Future<bool> stop() {
    throw UnimplementedError('stop() has not been implemented.');
  }

  Future<bool> seekTo(Duration position) {
    throw UnimplementedError('seekTo() has not been implemented.');
  }

  Future<bool> skipToQueueItem(int id) {
    throw UnimplementedError('skipToQueueItem() has not been implemented.');
  }
}
