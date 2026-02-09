## 0.0.2

### Added
- **Windows platform support** using System Media Transport Controls (SMTC) API
  - Supports `mediaStream`, `positionStream`, and playback controls (`playPause`, `skipToNext`, `skipToPrevious`, `stop`, `seekTo`)
  - Note: Queue-related features (`queueStream`, `getQueue()`, `skipToQueueItem()`) are not available on Windows

### Changed
- Updated README with platform compatibility table and Windows-specific documentation

## 0.0.1

- Initial release with Android support via MediaSession API
