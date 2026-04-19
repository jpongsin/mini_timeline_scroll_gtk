# Changelog

All notable changes to this project will be documented in this file. \
\
Note that the first three versions (0.0.0 - 0.2.0) were simultaneously released on April 17, 2026\
to make previous versions available for comparison.

## [0.3.1] - 2026-04-19
### Added
- Experimental drag and drop mouse controlled slider
- Refreshing metadata when loading another video is now selective rather than exhaustive
- Allowing videos with no subtitle tracks to have the options to add subtitles

### Changed
- Refactored the UI side for better troubleshooting
- Renamed hardware and software acceleration options for better clarification
- Reinforced and derived from frames for timecode calculation
- Refined WASD panning limits
- Video backend performance and thorough testing for fps to account for unusual codecs
- Disable audio option if no stream.


## [0.3.0] - 2026-04-18
### Added
- Ability to interact with subtitles; load a video with pre-existing subtitles, import an external subtitle and delete an external subtitle. Tested on mp4 and mkv videos.

### Changed
- Fixed memory issues with libmpv metadata retrieval from importing video.
- Refactored audio and subtitle implementations in Qt GUI context and backend context for better troubleshooting.

  
## [0.2.0] - 2026-04-17
### Added
- Limited support for Windows 10 (Tested working)
- Feature to disable hotkeys when app is in idle state and has no loaded videos
- libmpv parameters to pause the idle video for performance tweaks
- Feature to set screenshot folder
- Feature to toggle uninterrupted mode for screenshot functionality
- Codec details (useful for debug)


### Changed
- Migrated from gstreamer to libmpv to leverage wider compatibility of video formats
- As a result of the migration to libmpv, the codes have been condensed to allow finetuning of other features
- By using libmpv and Qt6, the responsiveness of windows opening and closing has improved crossplatform. On v0.1.0 gstreamer and Qt6 communicated adequately in this area on Linux and FreeBSD, but MacOS returned an unsuccessful close and subsequently unsuccessful and unpredictable opening app behavior.
- Zoom and pan now visible whether playing or pausing video
- Hardware and software acceleration is now toggled at the file menu
- Fine tuned zoom and pan features
- Reassigned autohide UI hotkey to h
- Reassigned zoom hotkeys; i for reset, o for shrink, and p for enlarge
- Moved timecode to the bottom left to make room for the scrolling bar
- Made window scaleable again
  
### Removed
- XWayland workaround. Support for Wayland is official. (Tested working on Linux with Wayland)
- Platform specific image formats; for now, sticking to png, webp, jpg and tiff for cross-platform
- Dark GUI style. Program defaults to Qt6's system colors. In testing varying platforms, Windows 10 was the only platform that was not responsive to the initial UI styling. It will be brought back in later versions.


## [0.1.0] - 2026-04-14

### Added
- Support for FreeBSD (tested working)
- Limited Wayland support with XWayland; initial testing showed that gstreamer was not responsive to Qt6 in Wayland, but was stable in X11 and Cocoa.
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
- Attempted prioritizing codecs in order to leverage wider compatibility of video formats

### Changed

- Migrated from GTK to Qt6 to keep UI design consistent after successful build but botched runtime on MacOS from v0.0.0.
- Reinforced MacOS support (building and compiling from source is recommended)
- Enlarged timecode
- Constrained window to video overlay for GUI
  
### Removed

- Hotkey notices. They have now been moved to file menu where hotkeys are referenced

## [0.0.0] - 2026-03-26

### Added
- Initial support for Linux
- GTK GUI in C++
- Dark GUI style
- gstreamer overlay in C; original intent was to tailor video stream to multiple platforms, optimize for high resolution videos and make room for other features
- Command line argument to play video e.g. "./mini_timeline_scroll sample.mov"
- Keyboard hotkeys to trigger playback, scroll, mute, rewind back, and fullscreen
- Carried over an ffmpeg metadata retrieval implementation from repo [jpongsin/metadata_fetch] and refactored source code into a retrieval task that returns fps information for the GUI to handle
- Timecode on a GTK GUI to monitor video duration

[0.3.1]: https://github.com/jpongsin/mini_timeline_scroll/compare/stable-0.3.0..stable-0.3.1
[0.3.0]: https://github.com/jpongsin/mini_timeline_scroll/compare/stable..stable-0.3.0
[0.2.0]: https://github.com/jpongsin/mini_timeline_scroll/compare/prototype..stable
[0.1.0]: https://github.com/jpongsin/mini_timeline_scroll/compare/legacy..prototype
[0.0.0]: https://github.com/jpongsin/mini_timeline_scroll/releases/tag/legacy
[jpongsin/metadata_fetch]: https://github.com/jpongsin/metadata_fetch
