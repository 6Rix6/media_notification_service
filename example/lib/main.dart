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

  MediaInfo? _currentMedia;
  PositionInfo? _position;
  bool _hasPermission = false;

  @override
  void initState() {
    super.initState();
    _initService();
    _checkPermission();
  }

  void _checkPermission() async {
    final hasPermission = await _service.hasPermission();
    setState(() {
      _hasPermission = hasPermission;
    });
  }

  void _openSettings() async {
    await _service.openSettings();
    _checkPermission();
  }

  void _initService() {
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
  }

  @override
  void dispose() {
    _mediaSub?.cancel();
    _positionSub?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: Scaffold(
        appBar: AppBar(title: Text('Media Notification Service')),
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            spacing: 16,
            children: [
              if (!_hasPermission)
                ElevatedButton(
                  onPressed: _openSettings,
                  child: Text('Open Permission Settings'),
                ),

              // album art (Uint8List)
              if (_currentMedia?.albumArt != null)
                Image.memory(
                  _currentMedia!.albumArt!,
                  height: 300,
                  width: 300,
                  fit: BoxFit.cover,
                ),

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
            ],
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
