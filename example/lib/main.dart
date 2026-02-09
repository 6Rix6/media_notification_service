import 'dart:async';

import 'package:flutter/material.dart';
import 'package:media_notification_service/media_notification_service.dart';

void main() {
  runApp(MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  final _service = MediaNotificationService();

  // subscriptions
  StreamSubscription? _mediaSub;
  StreamSubscription? _positionSub;
  StreamSubscription? _queueSub;

  MediaInfo? _currentMedia;
  PositionInfo? _position;
  List<QueueItem?>? _queue;
  bool _hasPermission = false;

  @override
  void initState() {
    super.initState();
    _initService();
  }

  Future<void> _checkPermission() async {
    final hasPermission = await _service.hasPermission();
    setState(() {
      _hasPermission = hasPermission;
    });
  }

  Future<void> _openSettings() async {
    await _service.openSettings();
    await _checkPermission();
  }

  void _initService() async {
    // Listen to media changes
    _mediaSub = _service.mediaStream.listen((data) {
      if (data != null) {
        // avoid unnecessary album art updates
        final shouldUpdateArt =
            _currentMedia?.albumArt == null || data.songChanged;
        setState(() {
          if (shouldUpdateArt) {
            _currentMedia = data.mediaInfo;
          } else {
            _currentMedia = data.mediaInfo.copyWith(
              albumArt: _currentMedia?.albumArt,
            );
          }
        });
      }
    });

    // Listen to position updates
    _positionSub = _service.positionStream.listen((position) {
      setState(() {
        _position = position;
      });
    });

    // Listen to queue updates
    _queueSub = _service.queueStream.listen((queue) {
      setState(() {
        _queue = queue;
      });
    });

    // check permission
    await _checkPermission();

    // get initial data
    if (_hasPermission) {
      final media = await _service.getCurrentMedia();
      final queue = await _service.getQueue();
      setState(() {
        _currentMedia = media;
        _queue = queue;
      });
    }
  }

  // force a data reload
  void _reloadData() async {
    await _checkPermission();
    if (_hasPermission) {
      final media = await _service.getCurrentMedia();
      final queue = await _service.getQueue();
      setState(() {
        _currentMedia = media;
        _queue = queue;
      });
    }
  }

  @override
  void dispose() {
    _mediaSub?.cancel();
    _positionSub?.cancel();
    _queueSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(
          title: Text('Media Notification Service'),
          actions: [
            IconButton(icon: Icon(Icons.refresh), onPressed: _reloadData),
          ],
        ),
        body: Center(
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              spacing: 8,
              children: [
                if (!_hasPermission)
                  ElevatedButton(
                    onPressed: _openSettings,
                    child: Text('Open Permission Settings'),
                  ),

                // album art (Uint8List)
                if (_currentMedia?.albumArt != null)
                  Image.memory(_currentMedia!.albumArt!, height: 200),

                // title and artist
                Text(
                  _currentMedia?.title ?? 'No media',
                  textAlign: TextAlign.center,
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
                Text(_currentMedia?.artist ?? '', textAlign: TextAlign.center),

                // progress slider
                Slider(
                  value: _position?.progress ?? 0, // normalized 0 to 1
                  onChanged: (value) {
                    if (_position == null) return;
                    final newPosition = Duration(
                      milliseconds: (value * _position!.duration.inMilliseconds)
                          .toInt(),
                    );
                    _service.seekTo(newPosition);
                  },
                ),

                // controls
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    IconButton(
                      icon: Icon(Icons.skip_previous, size: 32),
                      onPressed: () => _service.skipToPrevious(),
                    ),
                    IconButton(
                      icon: PlayPauseIcon(state: _currentMedia?.state),
                      onPressed: () => _service.playPause(),
                    ),
                    IconButton(
                      icon: Icon(Icons.skip_next, size: 32),
                      onPressed: () => _service.skipToNext(),
                    ),
                  ],
                ),

                // queue
                if (_queue != null && _queue!.isNotEmpty) ...[
                  Divider(),
                  Expanded(
                    child: ListView.builder(
                      itemCount: _queue!.length,
                      itemBuilder: (context, index) {
                        final item = _queue![index];
                        final isActive = index == _currentMedia?.queueIndex;
                        return Material(
                          color: isActive ? Colors.grey[200] : null,
                          child: ListTile(
                            leading: QueueArt(item: item),
                            title: Text(item?.title ?? 'Unknown'),
                            subtitle: Text(item?.artist ?? ''),
                            onTap: () {
                              final id = item?.id;
                              if (id != null) {
                                _service.skipToQueueItem(id);
                              }
                            },
                          ),
                        );
                      },
                    ),
                  ),
                ],
              ],
            ),
          ),
        ),
      ),
    );
  }
}

class PlayPauseIcon extends StatelessWidget {
  final PlaybackState? state;
  const PlayPauseIcon({super.key, required this.state});

  @override
  Widget build(BuildContext context) {
    if (state == PlaybackState.buffering) {
      return CircularProgressIndicator();
    }
    return Icon(
      state?.isPlaying == true ? Icons.pause : Icons.play_arrow,
      size: 48,
    );
  }
}

class QueueArt extends StatelessWidget {
  final QueueItem? item;
  const QueueArt({super.key, required this.item});

  @override
  Widget build(BuildContext context) {
    if (item == null) return Icon(Icons.music_note);
    final albumArt = item!.albumArt;
    final albumArtUri = item!.albumArtUri;
    if (albumArt != null) {
      return Image.memory(albumArt, width: 40);
    } else if (albumArtUri != null) {
      // queue image could be uri (file or http)
      return Image.network(albumArtUri.toString(), width: 40);
    } else {
      return Icon(Icons.music_note);
    }
  }
}
