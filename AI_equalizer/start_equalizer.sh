#!/bin/bash

# AI Equalizer Startup Script
# Sets up PulseAudio sinks, loopback modules, and routes Chrome audio

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== AI Equalizer Setup ==="
echo

# Function to check if a sink exists
sink_exists() {
    pactl list sinks short | grep -q "$1"
}

# Function to check if a module is loaded
module_exists() {
    pactl list modules short | grep -q "$1"
}

# Step 1: Create Equalizer_Input sink (if not exists)
if sink_exists "Equalizer_Input"; then
    echo "✓ Equalizer_Input sink already exists"
else
    echo "Creating Equalizer_Input sink..."
    pactl load-module module-null-sink sink_name=Equalizer_Input sink_properties=device.description="Equalizer_Input" rate=44100 channels=2
    echo "✓ Equalizer_Input sink created"
fi

# Step 2: Create Equalizer_Output sink (if not exists)
if sink_exists "Equalizer_Output"; then
    echo "✓ Equalizer_Output sink already exists"
else
    echo "Creating Equalizer_Output sink..."
    pactl load-module module-null-sink sink_name=Equalizer_Output sink_properties=device.description="Equalizer_Output" rate=44100 channels=2
    echo "✓ Equalizer_Output sink created"
fi

# Step 3: Set up loopback from Equalizer_Output to default sink (RDPSink)
DEFAULT_SINK=$(pactl get-default-sink)
echo "Default sink: $DEFAULT_SINK"

if module_exists "source=Equalizer_Output.monitor.*sink=$DEFAULT_SINK"; then
    echo "✓ Loopback to $DEFAULT_SINK already exists"
else
    echo "Creating loopback: Equalizer_Output.monitor → $DEFAULT_SINK..."
    pactl load-module module-loopback source=Equalizer_Output.monitor sink="$DEFAULT_SINK" latency_msec=1
    echo "✓ Loopback created"
fi

echo
echo "=== Audio Setup Complete ==="
echo

# Step 4: Find Chrome sink input and route to Equalizer_Input
echo "Looking for Chrome audio..."
CHROME_SINK_INPUT=$(pactl list sink-inputs | grep -B 20 "application.name.*Chrome" | grep "Sink Input #" | grep -oP '#\K[0-9]+')

if [ -z "$CHROME_SINK_INPUT" ]; then
    echo "⚠ Chrome is not currently playing audio."
    echo ""
    echo "To route Chrome when it starts playing, run:"
    echo "  pactl move-sink-input \$(pactl list sink-inputs | grep -B 20 \"application.name.*Chrome\" | grep \"Sink Input #\" | grep -oP '#\\K[0-9]+') Equalizer_Input"
else
    echo "Found Chrome sink input: #$CHROME_SINK_INPUT"
    echo "Routing Chrome → Equalizer_Input..."
    pactl move-sink-input "$CHROME_SINK_INPUT" Equalizer_Input
    echo "✓ Chrome audio routed to equalizer"
fi

echo
echo "=== Starting AI Equalizer ==="
echo

# Step 5: Start the equalizer application
if [ ! -f "$BUILD_DIR/AI_equalizer" ]; then
    echo "Error: AI_equalizer binary not found in $BUILD_DIR"
    echo "Please build the project first: cd build && cmake .. && make"
    exit 1
fi

cd "$BUILD_DIR"
./AI_equalizer &
APP_PID=$!

echo "✓ AI Equalizer started (PID: $APP_PID)"
echo
echo "Audio flow:"
echo "  Chrome → Equalizer_Input → parec → EQ Processing → Equalizer_Output → $DEFAULT_SINK → Speakers"
echo
echo "Click 'Start Audio' in the GUI to begin processing."
echo
echo "To stop: pkill AI_equalizer"
