#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <QObject>
#include <QAudioSink>
#include <QMediaDevices>
#include <QAudioFormat>
#include <QProcess>
#include <QThread>
#include <QQueue>
#include <QMutex>
#include <pulse/simple.h>
#include <pulse/error.h>
#include "equalizerengine.h"

/**
 * @class AudioProcessor
 * @brief Real-time audio capture and processing engine using PulseAudio's parec
 * 
 * This class implements a hybrid approach for audio capture in WSL/RDP environments:
 * - Capture: Uses external 'parec' process (direct PulseAudio API access)
 * - Processing: Routes audio through EqualizerEngine (10-band biquad filters)
 * - Output: Uses Qt's QAudioSink for playback
 * 
 * Architecture Decision:
 * Qt's QAudioSource cannot access PulseAudio monitor sources in WSL/RDP environments.
 * The parec utility has native PulseAudio API access and works reliably.
 * 
 * Audio Flow:
 * Chrome → Equalizer_Input (sink) → .monitor (source) → parec → EQ → RDPSink → speakers
 */
class AudioProcessor : public QObject
{
    Q_OBJECT

public:
    // Audio format constants
    static constexpr int SAMPLE_RATE = 44100;      // 44.1kHz CD-quality audio
    static constexpr int CHANNEL_COUNT = 2;         // Stereo
    static constexpr int PROCESS_INTERVAL_MS = 20;  // Unused in threaded mode (kept for compatibility)
    static constexpr int STARTUP_TIMEOUT_MS = 2000; // Max wait for parec to start
    static constexpr int SHUTDOWN_TIMEOUT_MS = 1000;// Max wait for graceful termination
    
    // PulseAudio device names
    static constexpr const char* MONITOR_SOURCE = "Equalizer_Input.monitor";
    static constexpr const char* OUTPUT_SINK_KEYWORD = "Equalizer_Output";
    
    explicit AudioProcessor(EqualizerEngine* equalizer, QObject *parent = nullptr);
    ~AudioProcessor() override;
    
    /**
     * @brief Start audio capture and processing pipeline
     * @return true if started successfully, false on error
     * 
     * Initializes:
     * 1. Finds RDPSink output device
     * 2. Starts QAudioSink for playback
     * 3. Launches parec process to capture from Equalizer_Input.monitor
     * 4. Starts timer for periodic audio processing
     */
    bool start();
    
    /**
     * @brief Stop audio processing and release all resources
     * 
     * Ensures clean shutdown:
     * 1. Stops processing timer
     * 2. Terminates parec process (graceful → forced)
     * 3. Stops audio sink
     * 4. Cleans up all allocated resources
     */
    void stop();
    
    bool isRunning() const { return m_running; }
    QString getLastError() const { return m_lastError; }
    
private slots:
    void onParecError(QProcess::ProcessError error);
    void onParecFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
private:
    // Core components
    EqualizerEngine* m_equalizer;
    QAudioFormat m_format;
    QThread* m_readThread{nullptr};    // Thread to read from parec
    QThread* m_writeThread{nullptr};   // Thread to process+write
    
    // Audio capture (parec process)
    QProcess* m_parecProcess;
    
    // Audio output (PulseAudio simple API)
    pa_simple* m_paOutput;
    
    // Processing control
    bool m_running;
    QString m_lastError;
    
    // Statistics (for debugging)
    qint64 m_totalBytesProcessed;
    int m_processingCycles;
    
    // Thread-safe queue between reader and writer
    QQueue<QByteArray> m_audioQueue;
    mutable QMutex m_queueMutex;
    QByteArray m_writeBuffer; // accumulation buffer in writer thread
    static constexpr int MIN_BUFFER_SIZE = 8820;  // ~50ms at 44.1kHz stereo (1102.5 frames × 8 bytes)
    static constexpr int PREBUFFER_BYTES = SAMPLE_RATE * CHANNEL_COUNT * sizeof(float); // ~1s prebuffer
    
    void setupAudioFormat();
    void setError(const QString& error);
    bool validateAudioFormat() const;
    void readAudioLoop();
    void writeAudioLoop();
};

#endif // AUDIOPROCESSOR_H
