# AI Equalizer - 10-Band Audio Equalizer with Qt UI

A professional real-time audio equalizer with 10 parametric bands, built using Qt6 with MVVM architecture and Qt Designer UI integration.

## Features

- 🎚️ **10-Band Parametric Equalizer**: Control frequencies from 31Hz to 16kHz
- 🎵 **Real-time Audio Processing**: Separate thread prevents UI freezing
- 🎨 **Qt Designer UI**: Visual layout editor support for easy customization
- 📋 **10 Factory Presets**: Rock, Pop, Jazz, Classical, Bass Boost, and more
- 🔄 **MVVM Architecture**: Clean separation of UI, model, and processing
- 🧵 **Thread-safe**: QMutex-protected state management
- 💾 **Preset Management**: Save and load custom EQ configurations

## Building the Project

### Prerequisites
```bash
sudo apt-get install build-essential cmake qt6-base-dev qt6-multimedia-dev qtcreator
```

### Option 1: Command Line Build
```bash
cd AI_equalizer
mkdir build && cd build
cmake ..
make -j$(nproc)
./AI_equalizer
```

### Option 2: Qt Creator (Recommended for Development)
1. Install Qt Creator:
   ```bash
   sudo apt-get install qtcreator
   ```

2. Open the project:
   ```bash
   qtcreator CMakeLists.txt
   ```
   Or: File → Open File or Project → Select `CMakeLists.txt`

3. Configure the project (click "Configure Project")

4. Build and run with Ctrl+R or click the green play button

## Editing UI with Qt Designer

1. In Qt Creator, navigate to `src/EqualizerMainWindow.ui` in the project tree
2. Double-click to open in Qt Designer (integrated in Qt Creator)
3. Drag and drop widgets, modify properties visually
4. Save and rebuild - changes apply automatically

See `QT_DESIGNER_GUIDE.md` for detailed instructions.

## Project Structure

```
AI_equalizer/
├── CMakeLists.txt                      # Qt Creator compatible CMake project
├── src/
│   ├── main.cpp                        # Application entry point
│   ├── EqualizerMainWindow.h/cpp       # Main window (View + Controller)
│   ├── EqualizerMainWindow.ui          # Qt Designer UI layout
│   ├── EqualizerViewModel.h/cpp        # Data model (MVVM pattern)
│   ├── AudioProcessingThread.h/cpp     # Background audio processing
│   ├── equalizerengine.h/cpp           # DSP: 10-band IIR filters
│   ├── audioprocessor.h/cpp            # Qt Multimedia integration
│   ├── PresetModel.h/cpp               # Preset management
│   └── ChatView.h/cpp                  # Chat UI (placeholder)
├── build/                              # Build directory (auto-generated)
└── *.md                                # Documentation files
```

## Architecture

### MVVM Pattern
- **Model** (`EqualizerViewModel`): Thread-safe state storage
- **View** (`EqualizerMainWindow`): Qt widgets and UI layout
- **Processing** (`AudioProcessingThread`): Separate QThread for audio

### Signal Flow
```
Audio Input → AudioProcessor → EqualizerEngine → Audio Output
                                       ↓
                              EqualizerViewModel
                                       ↓
                              EqualizerMainWindow (UI updates)
```

### Thread Safety
- Main UI thread: User interaction, widget updates
- Audio processing thread: Real-time DSP, audio I/O
- Communication: Qt signals/slots with queued connections
- State protection: QMutex in ViewModel

## EQ Bands

| Band | Frequency | Range    |
|------|-----------|----------|
| 1    | 31.25 Hz  | -12/+12 dB |
| 2    | 62.5 Hz   | -12/+12 dB |
| 3    | 125 Hz    | -12/+12 dB |
| 4    | 250 Hz    | -12/+12 dB |
| 5    | 500 Hz    | -12/+12 dB |
| 6    | 1 kHz     | -12/+12 dB |
| 7    | 2 kHz     | -12/+12 dB |
| 8    | 4 kHz     | -12/+12 dB |
| 9    | 8 kHz     | -12/+12 dB |
| 10   | 16 kHz    | -12/+12 dB |

## Factory Presets

1. **Flat** - No EQ adjustment
2. **Rock** - Enhanced bass and treble
3. **Pop** - Vocal emphasis with bass boost
4. **Jazz** - Smooth mids with controlled highs
5. **Classical** - Natural, balanced response
6. **Bass Boost** - Heavy low-end enhancement
7. **Treble Boost** - Crisp high frequencies
8. **Vocal** - Mid-range focus for speech
9. **Electronic** - Enhanced lows and highs for EDM
10. **Acoustic** - Balanced for acoustic instruments

## Requirements

- **OS**: Linux (Ubuntu 20.04+), Windows, macOS
- **Qt**: Qt6 (6.2+)
- **CMake**: 3.16+
- **Compiler**: C++17 compatible (GCC 7+, Clang 5+, MSVC 2017+)
- **Audio**: PulseAudio/ALSA (Linux), CoreAudio (macOS), WASAPI (Windows)

## Development

### Opening in Qt Creator
```bash
qtcreator /path/to/AI_equalizer/CMakeLists.txt
```

### Hot Keys in Qt Creator
- **Ctrl+B**: Build project
- **Ctrl+R**: Run application
- **F4**: Switch between header/source
- **Ctrl+K**: Open locator (quick file navigation)
- **F2**: Follow symbol under cursor

### Code Navigation
- All classes follow clear naming conventions
- ViewModel suffix: Data models
- Thread suffix: Background processing
- View suffix: UI-only widgets
- Model suffix: Business logic

## Troubleshooting

**No audio input/output**:
- Check system audio settings
- Ensure PulseAudio is running (Linux): `pulseaudio --check`
- Verify audio permissions

**Build errors**:
- Ensure all Qt6 modules are installed
- Clean rebuild: `rm -rf build && mkdir build && cd build && cmake .. && make`
- Check CMake output for missing dependencies

**UI not updating**:
- Verify thread-safe connections are established
- Check Qt signals/slots connections in constructor

## License

[Add your license here]

## Contributing

Contributions welcome! Please ensure:
- Code follows existing naming conventions
- UI changes are made in `.ui` files when possible
- Thread-safe patterns are maintained
- No blocking operations in UI thread

## Author

[Your name/organization]
