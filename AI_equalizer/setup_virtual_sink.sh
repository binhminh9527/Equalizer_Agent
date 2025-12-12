#!/bin/bash
# Setup script for virtual audio sink (PipeWire/PulseAudio compatible)
# Creates a dedicated "Equalizer_Input" device that apps can play to

set -e

echo "=========================================="
echo "  Equalizer Virtual Sink Setup"
echo "=========================================="
echo ""

# Detect audio system
if systemctl --user is-active pipewire.service &>/dev/null; then
    AUDIO_SYSTEM="pipewire"
    echo "Detected: PipeWire"
elif systemctl --user is-active pulseaudio.service &>/dev/null || pgrep -x pulseaudio &>/dev/null; then
    AUDIO_SYSTEM="pulseaudio"
    echo "Detected: PulseAudio"
else
    echo "ERROR: Neither PipeWire nor PulseAudio detected"
    echo "Make sure your audio system is running"
    exit 1
fi

echo ""

# Install pactl if needed (works with both PipeWire and PulseAudio)
if ! command -v pactl &>/dev/null; then
    echo "Installing required packages..."
    if [ "$AUDIO_SYSTEM" = "pipewire" ]; then
        sudo apt-get install -y pipewire-pulse
    else
        sudo apt-get install -y pulseaudio-utils
    fi
fi

# Function to create virtual sink
create_virtual_sink() {
    echo "Creating virtual audio device: 'Equalizer_Input'..."
    
    # Remove existing sink if present
    EXISTING_SINK=$(pactl list short sinks | grep "Equalizer_Input" | awk '{print $1}' || true)
    if [ -n "$EXISTING_SINK" ]; then
        echo "Removing existing Equalizer_Input sink..."
        pactl unload-module "$EXISTING_SINK" 2>/dev/null || true
    fi
    
    # Create null sink (virtual audio device)
    pactl load-module module-null-sink \
        sink_name=Equalizer_Input \
        sink_properties=device.description="Equalizer_Input" \
        rate=44100 \
        channels=2
    
    echo "✓ Virtual sink 'Equalizer_Input' created"
}

# Create the virtual sink
create_virtual_sink

echo ""
echo "=========================================="
echo "  Setup Complete!"
echo "=========================================="
echo ""
echo "Available audio devices:"
pactl list sinks short | grep -E "Equalizer_Input|$(pactl get-default-sink)" || pactl list sinks short
echo ""
echo "Monitor sources (for equalizer input):"
pactl list sources short | grep -i monitor
echo ""
echo "┌─────────────────────────────────────────┐"
echo "│  How to Use:                            │"
echo "├─────────────────────────────────────────┤"
echo "│  1. Start the equalizer:                │"
echo "│     ./build/AI_equalizer                │"
echo "│                                         │"
echo "│  2. Click 'Start Audio' in equalizer   │"
echo "│                                         │"
echo "│  3. Route your apps to use              │"
echo "│     'Equalizer_Input' as output:        │"
echo "│                                         │"
echo "│     Option A: Per-app (recommended)     │"
echo "│       - Open app audio settings         │"
echo "│       - Select 'Equalizer_Input'        │"
echo "│                                         │"
echo "│     Option B: System-wide               │"
echo "│       pactl set-default-sink \\          │"
echo "│           Equalizer_Input               │"
echo "│                                         │"
echo "│  4. Adjust EQ sliders in real-time!     │"
echo "└─────────────────────────────────────────┘"
echo ""
echo "Audio Flow:"
echo "  Music App → Equalizer_Input → Equalizer → Your Speakers"
echo "              (virtual)          (processing)  (physical)"
echo ""
echo "To restore default audio (bypass equalizer):"
echo "  pactl set-default-sink @DEFAULT_SINK@"
echo ""
echo "To remove virtual sink later:"
echo "  pactl unload-module module-null-sink"
echo ""
