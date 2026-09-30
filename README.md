# Qt 音乐播放器 / Qt Audio Player

基于 Qt 6 / C++17 的本地音乐播放器，支持 LRC 歌词同步、卡拉 OK 高亮，以及实时音频波形和频谱显示。

A local desktop music player built with Qt 6 / C++17, with synchronized LRC lyrics, karaoke-style highlighting, and real-time waveform and spectrum visualization.

## 功能 / Features

- **播放与列表：**拖入或选择音频文件，自动跳过重复文件；支持搜索筛选、拖动排序、移除歌曲和列表循环。
  **Playback and queue:** import files by drag-and-drop or file picker, skip duplicates, search, reorder, remove tracks, and repeat the queue.
- **播放控制：**上一首/下一首、进度拖动、音量与静音、0.5–3.0 倍速；显示歌曲元数据和内嵌专辑封面。
  **Controls:** previous/next, seeking, volume, mute, and 0.5–3.0× speed, with track metadata and embedded artwork.
- **歌词：**自动读取同目录、同名的 `.lrc` 文件；支持多时间标签、时间偏移，以及 UTF-8、带 BOM 的 UTF-16 和 GB18030 编码；按当前行进度显示卡拉 OK 高亮。
  **Lyrics:** automatically load a matching `.lrc` beside the audio file; support multiple timestamps, offsets, UTF-8, BOM-marked UTF-16, and GB18030; highlight the current line as it progresses.
- **可视化：**通过 `QAudioBufferOutput` 获取音频，使用内置 radix-2 FFT 生成频谱和波形，无需 kissfft 或外部 FFT 库。
  **Visualization:** analyze audio from `QAudioBufferOutput` with the built-in radix-2 FFT; no kissfft or external FFT library is required.
- **错误处理：**显示播放错误，并在播放过程中尝试跳过无法播放的文件。
  **Error handling:** display playback errors and attempt to skip unplayable files during playback.

支持导入 MP3、M4A、AAC、WAV、FLAC、OGG、OPUS 等文件；实际解码能力取决于 Qt Multimedia 后端和所部署的编解码器。歌词高亮基于行时间，不包含逐字时间标注。

The file picker accepts MP3, M4A, AAC, WAV, FLAC, OGG, OPUS, and other audio files. Decoding depends on the Qt Multimedia backend and deployed codecs. Karaoke highlighting uses line timing, without word-level timestamps.

## 构建要求 / Build requirements

- Windows x64、Visual Studio 2022 C++ 工具链及 Windows SDK。以下命令使用 MSVC 和 Ninja。
  Windows x64, the Visual Studio 2022 C++ toolchain, and Windows SDK. The commands below use MSVC and Ninja.
- Qt **6.8 或更高版本**的 `msvc2022_64` 套件，包含 **Core、Core5Compat、Widgets、Multimedia、MultimediaWidgets**；运行测试还需要 **Test** 模块。
  Qt **6.8 or later**, using the `msvc2022_64` kit with **Core, Core5Compat, Widgets, Multimedia, MultimediaWidgets**, plus **Test** for tests.
- CMake **3.21+**、Ninja；两者均须在 `PATH` 中。项目使用 C++17。
  CMake **3.21+** and Ninja, both available on `PATH`. The project uses C++17.

`res.qrc` 引用的图片和应用图标均随源码提供。

The images and application icon referenced by `res.qrc` are included in the source tree.

## Windows 构建与测试 / Build and test on Windows

打开 **x64 Native Tools Command Prompt for VS 2022（CMD）**，进入项目根目录。将下面的 `C:\Qt\6.x.x\msvc2022_64` **占位路径替换为实际 Qt 套件路径**：

Open **x64 Native Tools Command Prompt for VS 2022 (CMD)** in the repository root. **Replace the placeholder** `C:\Qt\6.x.x\msvc2022_64` with your Qt kit directory:

```bat
set "QTDIR=C:\Qt\6.x.x\msvc2022_64"
set "PATH=%QTDIR%\bin;%PATH%"
cmake --preset qt-release
cmake --build --preset qt-release
ctest --preset qt-release
cmake --install build/qt-release --prefix dist
```

若使用已初始化 MSVC x64 环境的 PowerShell，将前两行替换为下面两行，其余命令相同：

In PowerShell with the MSVC x64 environment already initialized, replace the first two lines with:

```powershell
$env:QTDIR = 'C:\Qt\6.x.x\msvc2022_64'
$env:PATH = "$env:QTDIR\bin;$env:PATH"
```

`qt-release` 使用 Ninja、Release 配置和 `QTDIR`，并开启测试；构建目录为 `build/qt-release`。安装步骤将应用及 Qt 部署脚本收集的运行依赖写入 `dist`。测试覆盖歌词解析、编码、时间边界、歌词文件查找和反相立体声音频分析。

The `qt-release` preset uses Ninja, Release mode, and `QTDIR`, with tests enabled. Build output goes to `build/qt-release`; installation writes the application and runtime dependencies collected by Qt's deployment script to `dist`. Tests cover lyric parsing, encoding, timing boundaries, lyric file lookup, and analysis of opposite-phase stereo audio.

已验证：Windows x64、Qt 6.11.2、MSVC 19.50、CMake 4.1.5、Ninja 的全新 Release 构建；CTest 1/1 通过，包含 10 个功能测试用例。

Verified: a fresh Release build on Windows x64 with Qt 6.11.2, MSVC 19.50, CMake 4.1.5, and Ninja; CTest passed 1/1 test target containing 10 functional test cases.

也可在 Qt Creator 中打开 `CMakeLists.txt`，选择满足上述依赖的 MSVC x64 Kit 后配置、构建并运行。

Alternatively, open `CMakeLists.txt` in Qt Creator, select an MSVC x64 kit with the dependencies above, then configure, build, and run.

## 使用 / Usage

1. 点击添加按钮或将音频文件拖入歌曲列表，双击歌曲开始播放。
   Add audio files with the add button or drag them into the queue, then double-click a track to play.
2. 将歌词放在音频旁并保持同名，例如 `song.flac` 与 `song.lrc`。
   Place lyrics beside the audio with the same base name, for example `song.flac` and `song.lrc`.
3. 使用搜索框筛选歌曲，拖动列表条目调整顺序；从列表移除歌曲不会删除磁盘文件。
   Search to filter tracks and drag queue entries to reorder them. Removing a track from the queue does not delete its file.

快捷键 / Shortcuts: `Ctrl+O` 添加 / add files；`Ctrl+F` 搜索 / search；`Ctrl+M` 静音 / mute；列表中 `Enter` 播放、`Delete` 移除 / play or remove the selected track in the queue.

## 许可证 / License

[MIT License](LICENSE)
