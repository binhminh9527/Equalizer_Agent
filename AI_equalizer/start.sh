#!/bin/bash

# Complete startup script for AI Equalizer

echo "======================================"
echo "   AI Equalizer - Complete Setup"
echo "======================================"
echo ""

# Check GEMINI_API_KEY
if [ -z "$GEMINI_API_KEY" ]; then
    echo "⚠️  Warning: GEMINI_API_KEY not set"
    echo "The AI chat feature will not work without it."
    echo ""
    echo "To set it:"
    echo "  export GEMINI_API_KEY='your_api_key_here'"
    echo ""
    read -p "Continue anyway? (y/n) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Start the Gemini service in the background
echo "Starting Gemini AI Service..."
cd gemini-service

if [ ! -z "$GEMINI_API_KEY" ]; then
    go run main.go > ../service.log 2>&1 &
    SERVICE_PID=$!
    echo "Service started (PID: $SERVICE_PID)"
    echo "Logs: service.log"
    
    # Wait a moment for service to start
    sleep 2
    
    # Check if service is running
    if ps -p $SERVICE_PID > /dev/null; then
        echo "✓ Gemini service is running on http://localhost:8080"
    else
        echo "✗ Failed to start Gemini service"
        echo "Check service.log for details"
        exit 1
    fi
else
    echo "⚠️  Skipping service start (no API key)"
    SERVICE_PID=""
fi

cd ..

# Build if needed
if [ ! -f "build/AI_equalizer" ]; then
    echo ""
    echo "Building Qt application..."
    ./build.sh || { echo "Build failed"; exit 1; }
fi

echo ""
echo "======================================"
echo "Starting AI Equalizer application..."
echo "======================================"
echo ""
if [ ! -z "$SERVICE_PID" ]; then
    echo "Press Ctrl+C to stop both service and application"
    echo ""
fi

cd build
./AI_equalizer

# Cleanup - kill service when app exits
if [ ! -z "$SERVICE_PID" ]; then
    echo ""
    echo "Stopping Gemini service..."
    kill $SERVICE_PID 2>/dev/null
fi

echo "Done!"
