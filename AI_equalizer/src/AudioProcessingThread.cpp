#include "AudioProcessingThread.h"
#include <QDebug>

AudioProcessingThread::AudioProcessingThread(EqualizerViewModel* model, QObject *parent)
        : QThread(parent), m_model(model), m_equalizer(nullptr), 
            m_audioProcessor(nullptr), m_shouldStop(false)
{
    // Connect to model changes
    connect(m_model, &EqualizerViewModel::bandGainChanged,
            this, &AudioProcessingThread::onModelBandGainChanged, Qt::QueuedConnection);
    connect(m_model, &EqualizerViewModel::allGainsChanged,
            this, &AudioProcessingThread::onModelAllGainsChanged, Qt::QueuedConnection);
}

AudioProcessingThread::~AudioProcessingThread()
{
    stopAudio();
    wait(); // Wait for thread to finish
}

void AudioProcessingThread::startAudio()
{
    QMutexLocker locker(&m_mutex);
    if (!isRunning()) {
        m_shouldStop = false;
        start();
    }
}

void AudioProcessingThread::stopAudio()
{
    {
        QMutexLocker locker(&m_mutex);
        m_shouldStop = true;
    }
    
    // Exit the event loop
    quit();
    
    if (isRunning()) {
        wait(3000); // Wait up to 3 seconds
    }
}

bool AudioProcessingThread::isRunning() const
{
    return QThread::isRunning();
}

void AudioProcessingThread::run()
{
    // Create audio components in this thread
    m_equalizer = new EqualizerEngine();
    m_audioProcessor = new AudioProcessor(m_equalizer);
    
    // Initialize with current model state
    m_equalizer->setAllGains(m_model->getBandGains());
    
    // Start audio processing
    if (!m_audioProcessor->start()) {
        emit errorOccurred("Failed to start audio processor");
        delete m_audioProcessor;
        delete m_equalizer;
        m_audioProcessor = nullptr;
        m_equalizer = nullptr;
        return;
    }
    
    emit audioStarted();
    qDebug() << "Audio thread started";
    
    // Run Qt event loop to process timer events
    exec();
    
    qDebug() << "Audio thread event loop exited";
    
    // Cleanup
    if (m_audioProcessor) {
        m_audioProcessor->stop();
        delete m_audioProcessor;
        m_audioProcessor = nullptr;
    }
    
    if (m_equalizer) {
        delete m_equalizer;
        m_equalizer = nullptr;
    }
    
    emit audioStopped();
    qDebug() << "Audio thread stopped";
}

void AudioProcessingThread::onModelBandGainChanged(int band, double gain)
{
    if (m_equalizer) {
        m_equalizer->setBandGain(band, gain);
    }
}

void AudioProcessingThread::onModelAllGainsChanged(const QVector<double>& gains)
{
    if (m_equalizer) {
        m_equalizer->setAllGains(gains);
    }
}
