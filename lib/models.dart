import 'dart:typed_data';

enum PlaybackState {
  none(0),
  stopped(1),
  paused(2),
  playing(3),
  fastForwarding(4),
  rewinding(5),
  buffering(6),
  error(7),
  connecting(8),
  skippingToPrevious(9),
  skippingToNext(10),
  skippingToQueueItem(11);

  final int value;
  const PlaybackState(this.value);

  static PlaybackState fromInt(int? value) {
    return PlaybackState.values.firstWhere(
      (e) => e.value == value,
      orElse: () => PlaybackState.none,
    );
  }

  static PlaybackState fromString(String? stateName) {
    switch (stateName) {
      case 'STATE_STOPPED':
        return PlaybackState.stopped;
      case 'STATE_PAUSED':
        return PlaybackState.paused;
      case 'STATE_PLAYING':
        return PlaybackState.playing;
      case 'STATE_FAST_FORWARDING':
        return PlaybackState.fastForwarding;
      case 'STATE_REWINDING':
        return PlaybackState.rewinding;
      case 'STATE_BUFFERING':
        return PlaybackState.buffering;
      case 'STATE_ERROR':
        return PlaybackState.error;
      case 'STATE_CONNECTING':
        return PlaybackState.connecting;
      case 'STATE_SKIPPING_TO_PREVIOUS':
        return PlaybackState.skippingToPrevious;
      case 'STATE_SKIPPING_TO_NEXT':
        return PlaybackState.skippingToNext;
      case 'STATE_SKIPPING_TO_QUEUE_ITEM':
        return PlaybackState.skippingToQueueItem;
      case 'STATE_NONE':
      default:
        return PlaybackState.none;
    }
  }

  bool get isPlaying => this == PlaybackState.playing;
}

class MediaInfo {
  final String? title;
  final String? artist;
  final String? album;
  final String? packageName;
  final Uint8List? albumArt;
  final bool isPlaying;
  final PlaybackState state;

  MediaInfo({
    this.title,
    this.artist,
    this.album,
    this.packageName,
    this.albumArt,
    this.isPlaying = false,
    this.state = PlaybackState.none,
  });

  factory MediaInfo.fromMap(
    Map<dynamic, dynamic> map, {
    bool updateArt = true,
    MediaInfo? oldMedia,
  }) {
    assert(
      updateArt || oldMedia != null,
      'oldMedia must not be null if updateArt is false',
    );

    return MediaInfo(
      title: map['title'] as String?,
      artist: map['artist'] as String?,
      album: map['album'] as String?,
      packageName: map['packageName'] as String?,
      albumArt: updateArt ? map['albumArt'] as Uint8List? : oldMedia?.albumArt,
      isPlaying: map['isPlaying'] as bool? ?? false,
      state: PlaybackState.fromString(map['state'] as String?),
    );
  }

  @override
  String toString() {
    return 'MediaInfo(title: $title, artist: $artist, isPlaying: $isPlaying)';
  }
}

class MediaInfoWithQueue {
  final MediaInfo mediaInfo;
  final QueueItem? nextItem;
  final QueueItem? previousItem;
  final bool songChanged;
  final bool queueChanged;

  MediaInfoWithQueue({
    required this.mediaInfo,
    this.nextItem,
    this.previousItem,
    this.songChanged = false,
    this.queueChanged = false,
  });

  factory MediaInfoWithQueue.fromMap(Map<dynamic, dynamic> map) {
    return MediaInfoWithQueue(
      mediaInfo: MediaInfo.fromMap(map),
      nextItem: map['nextItem'] != null
          ? QueueItem.fromMap(map['nextItem'] as Map<dynamic, dynamic>)
          : null,
      previousItem: map['previousItem'] != null
          ? QueueItem.fromMap(map['previousItem'] as Map<dynamic, dynamic>)
          : null,
      songChanged: map['songChanged'] as bool? ?? false,
      queueChanged: map['queueChanged'] as bool? ?? false,
    );
  }
}

class QueueItem {
  final String? title;
  final String? artist;
  final Uint8List? albumArt;
  final Uri? albumArtUri;

  QueueItem({this.title, this.artist, this.albumArt, this.albumArtUri});

  factory QueueItem.fromMap(Map<dynamic, dynamic> map) {
    return QueueItem(
      title: map['title'] as String?,
      artist: map['artist'] as String?,
      albumArt: map['albumArt'] as Uint8List?,
      albumArtUri: map['albumArtUri'] != null
          ? Uri.parse(map['albumArtUri'] as String)
          : null,
    );
  }

  @override
  String toString() {
    return 'QueueItem(title: $title, artist: $artist)';
  }
}

class PositionInfo {
  final Duration position;
  final Duration duration;
  final double playbackSpeed;
  final PlaybackState state;

  PositionInfo({
    required this.position,
    required this.duration,
    this.playbackSpeed = 1.0,
    this.state = PlaybackState.none,
  });

  factory PositionInfo.fromMap(Map<dynamic, dynamic> map) {
    return PositionInfo(
      position: Duration(milliseconds: map['position'] as int? ?? 0),
      duration: Duration(milliseconds: map['duration'] as int? ?? 0),
      playbackSpeed: (map['playbackSpeed'] as num?)?.toDouble() ?? 1.0,
      state: PlaybackState.fromString(map['state'] as String?),
    );
  }

  double get progress {
    if (duration.inMilliseconds == 0) return 0.0;
    return position.inMilliseconds / duration.inMilliseconds;
  }

  @override
  String toString() {
    return 'PositionInfo(position: $position, duration: $duration, speed: $playbackSpeed)';
  }
}
