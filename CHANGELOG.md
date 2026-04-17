# Changelog

All notable changes to this project will be documented in this file. \
\
Note that the first three versions (0.0.0 - 0.2.0) were simultaneously released on April 17, 2026\
to make previous versions available for comparison.

## [0.2.0] - 2026-04-17
### Added
- Feature to disable hotkeys when app is in idle state and has no loaded videos
- libmpv parameters to pause the idle video for performance tweaks
- Feature to set screenshot folder
- Feature to toggle uninterrupted mode for screenshot functionality
- Codec details (useful for debug)

### Changed
- Migrated from gstreamer to libmpv to leverage wider compatibility of video formats
- Zoom and pan now visible whether playing or pausing video
- Hardware and software acceleration is now toggled at the file menu
- Fine tuned zoom and pan features
- Reassigned autohide UI hotkey to h
- Reassigned zoom hotokeys; i for reset, o for shrink, and p for enlarge
  
### Removed
- Platform specific image formats; for now, sticking to png, webp, jpg and tiff.
- Dark GUI style. Program defaults to Qt6's system colors.

## [0.1.0] - 2026-04-14

### Added
- Ability to change audio tracks
- Hardware and software acceleration
- Open file menu
- Idle state video when no video is loaded
- Ability to close video and retain window (Ctrl+W) until user quits program (Ctrl+Q)
- Screenshot feature
- Autohide UI assigned to F11
- Zoom features with accompanying hotkeys (i for shrink, o for reset, p for enlarge)
- Pan features with accompanying hotkeys (w for pan up, s for pan down, a for pan left, d for pan right)
- FPS indicator on top bar

### Changed

- Migrated from GTK to Qt6 to keep UI design consistent after successful build but botched runtime on MacOS from v0.0.0
  
### Removed

- Hotkey notices. They have now been moved to file menu where hotkeys are referenced

## [0.0.0] - 2026-03-26

### Added

- GTK GUI in C++
- Dark GUI style
- gstreamer overlay in C; original intent was to tailor video stream to multiple platforms, optimize for high resolution videos and make room for other features
- Command line argument to play video e.g. "./mini_timeline_scroll sample.mov"
- Keyboard hotkeys to trigger playback, scroll, mute, rewind back, and fullscreen
- Carried over an ffmpeg metadata retrieval implementation from [repo metadata_fetch] and refactored main.c into video_fetch.c
- Timecode on a GTK GUI to monitor video duration

[0.2.0]: https://github.com/jpongsin/mini_timeline_scroll/compare/stable..prototype
[0.1.0]: https://github.com/jpongsin/mini_timeline_scroll/compare/prototype..legacy
[repo metadata_fetch]: https://github.com/jpongsin/metadata_fetch
[0.0.0]: https://github.com/jpongsin/mini_timeline_scroll/releases/tag/legacy
