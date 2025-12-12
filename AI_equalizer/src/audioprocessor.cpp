#include "audioprocessor.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>

/**
 * AudioProcessor Implementation
 * 
 * This implementation uses PulseAudio's 'parec' utility for audio capture because:
 * 1. Qt's QAudioSource fails to enumerate PulseAudio monitor sources in WSL/RDP
 * 2. parec has direct PA API access and reliably captures from virtual sinks
 * 3. Monitor sources (.monitor suffix) provide zero-latency audio loopback
 * 
 * Prerequisites:
 * - PulseAudio/PipeWire running with module-null-sink loaded
 * - Virtual sink 'Equalizer_Input' created (via setup_virtual_sink.sh)
 * - parec utility installed (pulseaudio-utils package)
 * - Audio source routed to 'Equalizer_Input' sink
 */

AudioProcessor::AudioProcessor(EqualizerEngine* equalizer, QObject *parent)
    : QObject(parent)
    , m_equalizer(equalizer)
    , m_parecProcess(nullptr)
    , m_paOutput(nullptr)
    , m_readThread(nullptr)
    , m_writeThread(nullptr)
    , m_running(false)
    , m_totalBytesProcessed(0)
    , m_processingCycles(0)
{
    Q_ASSERT(m_equalizer != nullptr);
    setupAudioFormat();
}

AudioProcessor::~AudioProcessor()
{
    // Ensure clean shutdown even if stop() wasn't called
    if (m_running) {
        qWarning() << "AudioProcessor destroyed while running - forcing stop()";
        stop();
    }
}

void AudioProcessor::setupAudioFormat()
{
    // Configure audio format for 44.1kHz stereo float32
    // This matches PulseAudio's native format for zero-copy processing
    m_format.setSampleRate(SAMPLE_RATE);
    m_format.setChannelCount(CHANNEL_COUNT);
    m_format.setSampleFormat(QAudioFormat::Float);
    
    // Sync equalizer engine with our sample rate
    m_equalizer->setSampleRate(m_format.sampleRate());
    
    qDebug() << "Audio format configured:"
             << "Rate:" << m_format.sampleRate() << "Hz"
             << "| Channels:" << m_format.channelCount()
             << "| Format: Float32LE"
             << "| Frame size:" << m_format.bytesPerFrame() << "bytes";
}

bool AudioProcessor::start()
{
    if (m_running) {
        qDebug() << "Audio processor already running";
        return true;
    }
    
    qDebug() << "\n=== Starting Audio Processor ===";
    
    // Validate audio format before starting
    if (!validateAudioFormat()) {
        setError("Invalid audio format configuration");
        return false;
    }
    
    // Reset statistics
    m_totalBytesProcessed = 0;
    m_processingCycles = 0;
    m_lastError.clear();
    
    // Step 1: Find suitable output device (RDPSink for WSL/RDP environments)
    QAudioDevice outputDevice;
    QList<QAudioDevice> audioOutputs = QMediaDevices::audioOutputs();
    
    qDebug() << "Scanning for output devices...";
    qDebug() << "Available outputs (" << audioOutputs.size() << " total):";
    for (const QAudioDevice& device : audioOutputs) {
        QString desc = device.description();
        qDebug() << "  -" << desc << (device.isDefault() ? "[DEFAULT]" : "");
        
        // Prefer RDP sink for WSL/RDP, but remember any device as fallback
        if (desc.contains(OUTPUT_SINK_KEYWORD, Qt::CaseInsensitive)) {
            outputDevice = device;
            qDebug() << "    ^ Selected (matched keyword:" << OUTPUT_SINK_KEYWORD << ")";
        }
    }
    
    // Fallback to default output if no RDP sink found
    if (outputDevice.isNull() && !audioOutputs.isEmpty()) {
        outputDevice = QMediaDevices::defaultAudioOutput();
        qWarning() << "RDPSink not found, using default output:" << outputDevice.description();
    }
    
    if (outputDevice.isNull()) {
        setError("No audio output device available");
        return false;
    }
    
    // Step 2: Initialize PulseAudio output
    qDebug() << "\nInitializing PulseAudio output...";
    qDebug() << "Output sink:" << OUTPUT_SINK_KEYWORD;
    
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_FLOAT32LE;
    ss.rate = SAMPLE_RATE;
    ss.channels = CHANNEL_COUNT;
    
    pa_buffer_attr bufattr;
    bufattr.maxlength = (uint32_t) -1;
    bufattr.tlength = pa_usec_to_bytes(200000, &ss); // 200ms target latency (more stable)
    bufattr.prebuf = (uint32_t) -1;
    bufattr.minreq = (uint32_t) -1;
    
    int error;
    m_paOutput = pa_simple_new(
        nullptr,                    // Default server
        "AI_Equalizer",            // Application name
        PA_STREAM_PLAYBACK,        // Playback stream
        OUTPUT_SINK_KEYWORD,       // Sink name
        "Equalized Audio",         // Stream description
        &ss,                       // Sample format
        nullptr,                   // Default channel map
        &bufattr,                  // Buffer attributes
        &error                     // Error code
    );
    
    if (!m_paOutput) {
        setError(QString("Failed to create PulseAudio output: %1").arg(pa_strerror(error)));
        qCritical() << "pa_simple_new failed:" << pa_strerror(error);
        return false;
    }
    
    qDebug() << "PulseAudio output initialized successfully";
    qDebug() << "Format: float32le," << CHANNEL_COUNT << "ch," << SAMPLE_RATE << "Hz";
    
    // Step 3: Launch parec process to capture from monitor source
    qDebug() << "\nLaunching parec capture process...";
    qDebug() << "Monitor source:" << MONITOR_SOURCE;
    
    m_parecProcess = new QProcess(this);
    
    // Connect diagnostics only; processing handled by worker threads
    connect(m_parecProcess, &QProcess::errorOccurred, this, &AudioProcessor::onParecError);
    connect(m_parecProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AudioProcessor::onParecFinished);
    
    // Build parec command arguments
    QStringList args;
    args << QString("--device=%1").arg(MONITOR_SOURCE)
         << "--format=float32le"
         << QString("--rate=%1").arg(m_format.sampleRate())
         << QString("--channels=%1").arg(m_format.channelCount());
    
    qDebug() << "Command: parec" << args.join(" ");
    
    m_parecProcess->start("parec", args);
    
    if (!m_parecProcess->waitForStarted(STARTUP_TIMEOUT_MS)) {
        QString error = m_parecProcess->errorString();
        setError(QString("Failed to start parec: %1").arg(error));
        qCritical() << "parec startup failed after" << STARTUP_TIMEOUT_MS << "ms";
        qCritical() << "Error:" << error;
        qCritical() << "Make sure:";
        qCritical() << "  1. 'parec' is installed (apt install pulseaudio-utils)";
        qCritical() << "  2. Virtual sink exists (run setup_virtual_sink.sh)";
        qCritical() << "  3. PulseAudio/PipeWire is running";
        
        // Cleanup on failure
        if (m_paOutput) {
            pa_simple_free(m_paOutput);
            m_paOutput = nullptr;
        }
        delete m_parecProcess;
        m_parecProcess = nullptr;
        return false;
    }
    
    qDebug() << "parec process started (PID:" << m_parecProcess->processId() << ")";
    
    // Start worker threads
    m_running = true;
    m_readThread = QThread::create([this]{ readAudioLoop(); });
    m_writeThread = QThread::create([this]{ writeAudioLoop(); });
    m_readThread->start();
    m_writeThread->start();
    
    qDebug() << "\n✓ Audio processor started successfully";
    qDebug() << "Audio flow: " << MONITOR_SOURCE << "→ parec → EQ (C++) → PulseAudio →" 
             << OUTPUT_SINK_KEYWORD << "\n";
    
    return true;
}

void AudioProcessor::stop()
{
    if (!m_running) {
        qDebug() << "Audio processor already stopped";
        return;
    }
    
    qDebug() << "\n=== Stopping Audio Processor ===";
    
    // Stop threads first
    m_running = false;
    if (m_readThread) {
        m_readThread->quit();
        m_readThread->wait();
        delete m_readThread;
        m_readThread = nullptr;
    }
    if (m_writeThread) {
        m_writeThread->quit();
        m_writeThread->wait();
        delete m_writeThread;
        m_writeThread = nullptr;
    }
    {
        QMutexLocker lock(&m_queueMutex);
        m_audioQueue.clear();
    }
    m_writeBuffer.clear();
    
    // Print final statistics
    if (m_processingCycles > 0) {
        qDebug() << "Session statistics:";
        qDebug() << "  Total bytes processed:" << m_totalBytesProcessed;
        qDebug() << "  Processing cycles:" << m_processingCycles;
        qDebug() << "  Average bytes/cycle:" << (m_processingCycles ? (m_totalBytesProcessed / m_processingCycles) : 0);
    }
    
    // Step 1: Terminate parec process (graceful → forced)
    if (m_parecProcess) {
        qDebug() << "Terminating parec process...";
        
        // Disconnect signals to avoid callbacks during shutdown
        disconnect(m_parecProcess, nullptr, this, nullptr);
        
        // Try graceful termination first
        m_parecProcess->terminate();
        if (!m_parecProcess->waitForFinished(SHUTDOWN_TIMEOUT_MS)) {
            qWarning() << "parec didn't terminate gracefully, forcing kill...";
            m_parecProcess->kill();
            m_parecProcess->waitForFinished(); // Wait indefinitely for kill
        }
        
        qDebug() << "parec process terminated";
        delete m_parecProcess;
        m_parecProcess = nullptr;
    }
    
    // Step 3: Free PulseAudio output
    if (m_paOutput) {
        qDebug() << "Closing PulseAudio output...";
        pa_simple_drain(m_paOutput, nullptr);  // Drain remaining audio
        pa_simple_free(m_paOutput);
        m_paOutput = nullptr;
        qDebug() << "PulseAudio output closed";
    }
    
    // Step 4: Clear state
    m_running = false;
    
    qDebug() << "✓ Audio processor stopped cleanly\n";
}

void AudioProcessor::readAudioLoop()
{
    qDebug() << "Read thread started";
    const int bytesPerFrame = m_format.bytesPerFrame();
    if (bytesPerFrame <= 0) {
        qCritical() << "Invalid bytes per frame in read loop" << bytesPerFrame;
        return;
    }
    QByteArray chunkBuffer;
    while (m_running) {
        if (!m_parecProcess || m_parecProcess->state() != QProcess::Running) {
            QThread::msleep(10);
            continue;
        }
        QByteArray data = m_parecProcess->readAllStandardOutput();
        if (!data.isEmpty()) {
            // Align to frame boundary
            int alignedSize = (data.size() / bytesPerFrame) * bytesPerFrame;
            if (alignedSize <= 0) {
                QThread::msleep(5);
                continue;
            }
            chunkBuffer = data.left(alignedSize);
            const int frameCount = chunkBuffer.size() / bytesPerFrame;
            float* buffer = reinterpret_cast<float*>(chunkBuffer.data());
            m_equalizer->processBuffer(buffer, frameCount, m_format.channelCount());
            QMutexLocker lock(&m_queueMutex);
            m_audioQueue.enqueue(chunkBuffer);
        } else {
            QThread::msleep(5);
        }
    }
    qDebug() << "Read thread exiting";
}

void AudioProcessor::writeAudioLoop()
{
    qDebug() << "Write thread started";
    const int bytesPerFrame = m_format.bytesPerFrame();
    while (m_running) {
        // accumulate from queue
        {
            QMutexLocker lock(&m_queueMutex);
            while (!m_audioQueue.isEmpty()) {
                m_writeBuffer.append(m_audioQueue.dequeue());
            }
        }
        if (m_writeBuffer.size() < PREBUFFER_BYTES) {
            QThread::msleep(5);
            continue;
        }
        if (bytesPerFrame <= 0) {
            qCritical() << "Invalid bytes per frame" << bytesPerFrame;
            break;
        }
        int processSize = (m_writeBuffer.size() / bytesPerFrame) * bytesPerFrame;
        if (processSize <= 0) {
            QThread::msleep(5);
            continue;
        }
        QByteArray audioData = m_writeBuffer.left(processSize);
        m_writeBuffer.remove(0, processSize);
        const int frameCount = audioData.size() / bytesPerFrame;
        int error;
        if (pa_simple_write(m_paOutput, audioData.constData(), audioData.size(), &error) < 0) {
            qWarning() << "PulseAudio write error:" << pa_strerror(error);
            break;
        }
        m_totalBytesProcessed += audioData.size();
        m_processingCycles++;
    }
    qDebug() << "Write thread exiting";
}

void AudioProcessor::onParecError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error) {
        case QProcess::FailedToStart:
            errorMsg = "Failed to start (parec not found or insufficient permissions)";
            break;
        case QProcess::Crashed:
            errorMsg = "Process crashed";
            break;
        case QProcess::Timedout:
            errorMsg = "Process timed out";
            break;
        case QProcess::WriteError:
            errorMsg = "Write error";
            break;
        case QProcess::ReadError:
            errorMsg = "Read error";
            break;
        default:
            errorMsg = "Unknown error";
    }
    
    qCritical() << "parec process error:" << errorMsg;
    setError(QString("parec error: %1").arg(errorMsg));
    
    // Stop gracefully on error
    if (m_running) {
        stop();
    }
}

void AudioProcessor::onParecFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (!m_running) {
        // Normal shutdown, ignore
        return;
    }
    
    QString statusMsg = (exitStatus == QProcess::NormalExit) 
                        ? QString("exited normally with code %1").arg(exitCode)
                        : "crashed";
    
    qWarning() << "parec process" << statusMsg << "while audio processor was running";
    
    // Check stderr for error messages
    if (m_parecProcess) {
        QByteArray stderr = m_parecProcess->readAllStandardError();
        if (!stderr.isEmpty()) {
            qWarning() << "parec stderr:" << QString::fromUtf8(stderr).trimmed();
        }
    }
    
    setError(QString("parec %1").arg(statusMsg));
    stop();
}

void AudioProcessor::setError(const QString& error)
{
    m_lastError = error;
    qCritical() << "AudioProcessor error:" << error;
}

bool AudioProcessor::validateAudioFormat() const
{
    if (m_format.sampleRate() != SAMPLE_RATE) {
        qCritical() << "Invalid sample rate:" << m_format.sampleRate() 
                    << "(expected" << SAMPLE_RATE << ")";
        return false;
    }
    
    if (m_format.channelCount() != CHANNEL_COUNT) {
        qCritical() << "Invalid channel count:" << m_format.channelCount()
                    << "(expected" << CHANNEL_COUNT << ")";
        return false;
    }
    
    if (m_format.sampleFormat() != QAudioFormat::Float) {
        qCritical() << "Invalid sample format (expected Float32)";
        return false;
    }
    
    if (m_format.bytesPerFrame() <= 0) {
        qCritical() << "Invalid bytes per frame:" << m_format.bytesPerFrame();
        return false;
    }
    
    return true;
}
