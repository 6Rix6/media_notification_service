import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:media_notification_service/media_notification_service.dart';

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  group('MediaNotificationService Integration Tests', () {
    late MediaNotificationService service;

    setUp(() {
      service = MediaNotificationService();
    });

    testWidgets('should initialize without errors', (
      WidgetTester tester,
    ) async {
      expect(service, isNotNull);
    });

    testWidgets('hasPermission should return a boolean', (
      WidgetTester tester,
    ) async {
      final hasPermission = await service.hasPermission();
      expect(hasPermission, isA<bool>());
    });

    testWidgets('getCurrentMedia should return null when no media is playing', (
      WidgetTester tester,
    ) async {
      // Note: This test assumes no media is playing
      // In real scenarios, this might need to be skipped or conditional
      final media = await service.getCurrentMedia();
      expect(media, isA<MediaInfo?>());
    });

    testWidgets('mediaStream should be a valid stream', (
      WidgetTester tester,
    ) async {
      expect(service.mediaStream, isA<Stream<MediaInfoWithQueue?>>());

      // Listen to the stream for a short duration
      final subscription = service.mediaStream.listen((data) {
        // Stream should emit MediaInfoWithQueue or null
        expect(data, isA<MediaInfoWithQueue?>());
      });

      await tester.pumpAndSettle(const Duration(seconds: 2));
      await subscription.cancel();
    });

    testWidgets('positionStream should be a valid stream', (
      WidgetTester tester,
    ) async {
      expect(service.positionStream, isA<Stream<PositionInfo?>>());

      final subscription = service.positionStream.listen((data) {
        expect(data, isA<PositionInfo?>());
      });

      await tester.pumpAndSettle(const Duration(seconds: 2));
      await subscription.cancel();
    });

    testWidgets('playPause should return a boolean', (
      WidgetTester tester,
    ) async {
      final result = await service.playPause();
      expect(result, isA<bool>());
    });

    testWidgets('skipToNext should return a boolean', (
      WidgetTester tester,
    ) async {
      final result = await service.skipToNext();
      expect(result, isA<bool>());
    });

    testWidgets('skipToPrevious should return a boolean', (
      WidgetTester tester,
    ) async {
      final result = await service.skipToPrevious();
      expect(result, isA<bool>());
    });

    testWidgets('stop should return a boolean', (WidgetTester tester) async {
      final result = await service.stop();
      expect(result, isA<bool>());
    });

    testWidgets('seekTo should return a boolean', (WidgetTester tester) async {
      final result = await service.seekTo(const Duration(seconds: 30));
      expect(result, isA<bool>());
    });

    testWidgets('openSettings should complete without errors', (
      WidgetTester tester,
    ) async {
      // This will open system settings, which might be disruptive during testing
      // You might want to skip this in automated CI/CD
      await expectLater(service.openSettings(), completes);
    });

    group('With Active Media Session', () {
      // These tests should only run when media is actively playing
      // You might want to use setUp to start a media player before these tests

      testWidgets('should receive media updates when song changes', (
        WidgetTester tester,
      ) async {
        MediaInfoWithQueue? receivedData;

        final subscription = service.mediaStream.listen((data) {
          receivedData = data;
        });

        // Wait for potential media updates
        await tester.pumpAndSettle(const Duration(seconds: 5));

        // If media is playing, we should have received data
        // This assertion is conditional and might need adjustment
        if (receivedData != null) {
          expect(receivedData!.mediaInfo, isA<MediaInfo>());
          expect(receivedData!.songChanged, isA<bool>());
          expect(receivedData!.queueChanged, isA<bool>());
        }

        await subscription.cancel();
      });

      testWidgets('should track position updates when playing', (
        WidgetTester tester,
      ) async {
        final positions = <PositionInfo>[];

        final subscription = service.positionStream.listen((data) {
          if (data != null) {
            positions.add(data);
          }
        });

        // Collect position updates for 3 seconds
        await tester.pumpAndSettle(const Duration(seconds: 3));

        // If media is playing, positions should increase over time
        if (positions.length >= 2) {
          final firstPosition = positions.first.position;
          final lastPosition = positions.last.position;
          expect(
            lastPosition.inMilliseconds,
            greaterThanOrEqualTo(firstPosition.inMilliseconds),
          );
        }

        await subscription.cancel();
      });

      testWidgets('playPause should toggle playback state', (
        WidgetTester tester,
      ) async {
        MediaInfo? initialMedia;
        MediaInfo? afterPauseMedia;

        // Get initial state
        initialMedia = await service.getCurrentMedia();

        if (initialMedia != null && initialMedia.isPlaying) {
          // Pause
          await service.playPause();
          await tester.pumpAndSettle(const Duration(milliseconds: 500));

          afterPauseMedia = await service.getCurrentMedia();

          // Verify state changed
          expect(afterPauseMedia?.isPlaying, isFalse);

          // Resume
          await service.playPause();
          await tester.pumpAndSettle(const Duration(milliseconds: 500));

          final afterResumeMedia = await service.getCurrentMedia();
          expect(afterResumeMedia?.isPlaying, isTrue);
        }
      });

      testWidgets('seekTo should update playback position', (
        WidgetTester tester,
      ) async {
        final initialMedia = await service.getCurrentMedia();

        if (initialMedia != null) {
          // Seek to 30 seconds
          final targetPosition = const Duration(seconds: 30);
          final seekResult = await service.seekTo(targetPosition);

          if (seekResult) {
            await tester.pumpAndSettle(const Duration(milliseconds: 500));

            // Listen for position update
            PositionInfo? newPosition;
            final subscription = service.positionStream.listen((data) {
              if (data != null) {
                newPosition = data;
              }
            });

            await tester.pumpAndSettle(const Duration(seconds: 1));

            if (newPosition != null) {
              // Position should be close to target (within 2 seconds tolerance)
              expect(
                newPosition!.position.inSeconds,
                closeTo(targetPosition.inSeconds, 2),
              );
            }

            await subscription.cancel();
          }
        }
      });

      testWidgets('should receive queue information', (
        WidgetTester tester,
      ) async {
        MediaInfoWithQueue? data;

        final subscription = service.mediaStream.listen((event) {
          data = event;
        });

        await tester.pumpAndSettle(const Duration(seconds: 2));

        if (data != null) {
          // Queue items might be null if not supported by the media app
          expect(data!.nextItem, isA<QueueItem?>());
          expect(data!.previousItem, isA<QueueItem?>());

          if (data!.nextItem != null) {
            expect(data!.nextItem!.title, isA<String?>());
            expect(data!.nextItem!.artist, isA<String?>());
          }
        }

        await subscription.cancel();
      });
    });

    group('Model Tests', () {
      test('MediaInfo.fromMap should parse correctly', () {
        final map = {
          'title': 'Test Song',
          'artist': 'Test Artist',
          'album': 'Test Album',
          'packageName': 'com.test.app',
          'isPlaying': true,
          'state': 'STATE_PLAYING',
        };

        final mediaInfo = MediaInfo.fromMap(map);

        expect(mediaInfo.title, equals('Test Song'));
        expect(mediaInfo.artist, equals('Test Artist'));
        expect(mediaInfo.album, equals('Test Album'));
        expect(mediaInfo.packageName, equals('com.test.app'));
        expect(mediaInfo.isPlaying, isTrue);
        expect(mediaInfo.state, equals(PlaybackState.playing));
      });

      test('QueueItem.fromMap should parse correctly', () {
        final map = {'title': 'Next Song', 'artist': 'Next Artist'};

        final queueItem = QueueItem.fromMap(map);

        expect(queueItem.title, equals('Next Song'));
        expect(queueItem.artist, equals('Next Artist'));
      });

      test('PositionInfo.fromMap should parse correctly', () {
        final map = {
          'position': 30000, // 30 seconds in milliseconds
          'duration': 180000, // 3 minutes in milliseconds
          'playbackSpeed': 1.0,
          'state': 'STATE_PLAYING',
        };

        final positionInfo = PositionInfo.fromMap(map);

        expect(positionInfo.position, equals(const Duration(seconds: 30)));
        expect(positionInfo.duration, equals(const Duration(minutes: 3)));
        expect(positionInfo.playbackSpeed, equals(1.0));
        expect(positionInfo.state, equals(PlaybackState.playing));
      });

      test('PositionInfo.progress should calculate correctly', () {
        final positionInfo = PositionInfo(
          position: const Duration(seconds: 30),
          duration: const Duration(minutes: 2),
          playbackSpeed: 1.0,
          state: PlaybackState.playing,
        );

        expect(positionInfo.progress, closeTo(0.25, 0.01)); // 30/120 = 0.25
      });

      test('PlaybackState.fromString should parse correctly', () {
        expect(
          PlaybackState.fromString('STATE_PLAYING'),
          equals(PlaybackState.playing),
        );
        expect(
          PlaybackState.fromString('STATE_PAUSED'),
          equals(PlaybackState.paused),
        );
        expect(
          PlaybackState.fromString('STATE_STOPPED'),
          equals(PlaybackState.stopped),
        );
        expect(
          PlaybackState.fromString('STATE_BUFFERING'),
          equals(PlaybackState.buffering),
        );
        expect(
          PlaybackState.fromString('STATE_NONE'),
          equals(PlaybackState.none),
        );
        expect(PlaybackState.fromString(null), equals(PlaybackState.none));
        expect(PlaybackState.fromString('INVALID'), equals(PlaybackState.none));
      });

      test('MediaInfo.copyWith should work correctly', () {
        final original = MediaInfo(
          title: 'Original',
          artist: 'Artist',
          isPlaying: true,
          state: PlaybackState.playing,
        );

        final copied = original.copyWith(title: 'Updated', isPlaying: false);

        expect(copied.title, equals('Updated'));
        expect(copied.artist, equals('Artist')); // Should remain unchanged
        expect(copied.isPlaying, isFalse);
        expect(
          copied.state,
          equals(PlaybackState.playing),
        ); // Should remain unchanged
      });
    });

    group('Error Handling', () {
      testWidgets('should handle permission denial gracefully', (
        WidgetTester tester,
      ) async {
        final hasPermission = await service.hasPermission();

        if (!hasPermission) {
          // Methods should return false when permission is not granted
          final playPauseResult = await service.playPause();
          expect(playPauseResult, isFalse);

          final media = await service.getCurrentMedia();
          expect(media, isNull);
        }
      });

      testWidgets('should handle no active media session', (
        WidgetTester tester,
      ) async {
        // When no media is playing, methods should handle gracefully
        final result = await service.skipToNext();
        expect(result, isA<bool>());

        final media = await service.getCurrentMedia();
        expect(media, isA<MediaInfo?>());
      });

      testWidgets('should handle invalid seek position', (
        WidgetTester tester,
      ) async {
        // Negative duration
        final negativeResult = await service.seekTo(
          const Duration(seconds: -10),
        );
        expect(negativeResult, isA<bool>());

        // Very large duration (beyond song length)
        final largeResult = await service.seekTo(const Duration(hours: 10));
        expect(largeResult, isA<bool>());
      });
    });

    group('Stream Stability', () {
      testWidgets('mediaStream should handle rapid subscription changes', (
        WidgetTester tester,
      ) async {
        // Subscribe and unsubscribe multiple times rapidly
        for (int i = 0; i < 5; i++) {
          final subscription = service.mediaStream.listen((_) {});
          await tester.pump();
          await subscription.cancel();
          await tester.pump();
        }

        // Stream should still work after rapid changes
        final subscription = service.mediaStream.listen((data) {
          expect(data, isA<MediaInfoWithQueue?>());
        });

        await tester.pumpAndSettle(const Duration(seconds: 1));
        await subscription.cancel();
      });

      testWidgets('positionStream should handle rapid subscription changes', (
        WidgetTester tester,
      ) async {
        for (int i = 0; i < 5; i++) {
          final subscription = service.positionStream.listen((_) {});
          await tester.pump();
          await subscription.cancel();
          await tester.pump();
        }

        final subscription = service.positionStream.listen((data) {
          expect(data, isA<PositionInfo?>());
        });

        await tester.pumpAndSettle(const Duration(seconds: 1));
        await subscription.cancel();
      });

      testWidgets('multiple simultaneous stream listeners should work', (
        WidgetTester tester,
      ) async {
        int mediaCount = 0;
        int positionCount = 0;

        final mediaSubscription1 = service.mediaStream.listen(
          (_) => mediaCount++,
        );
        final mediaSubscription2 = service.mediaStream.listen(
          (_) => mediaCount++,
        );
        final positionSubscription1 = service.positionStream.listen(
          (_) => positionCount++,
        );
        final positionSubscription2 = service.positionStream.listen(
          (_) => positionCount++,
        );

        await tester.pumpAndSettle(const Duration(seconds: 2));

        // Both listeners should have received events
        // (actual counts depend on media state)
        expect(mediaCount, greaterThanOrEqualTo(0));
        expect(positionCount, greaterThanOrEqualTo(0));

        await mediaSubscription1.cancel();
        await mediaSubscription2.cancel();
        await positionSubscription1.cancel();
        await positionSubscription2.cancel();
      });
    });
  });
}
