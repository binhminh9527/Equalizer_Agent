#ifndef AUDIOPROCESSORTHREAD_H
#define AUDIOPROCESSORTHREAD_H

#include <QThread>
#include <QMutex>
#include <atomic>
#include "equalizerengine.h"
#include "audioprocessor.h"
#include "EqualizerViewModel.h"

// Runs audio processing in a separate thread
class AudioProcessingThread : public QThread
{
    Q_OBJECT

public:
    explicit AudioProcessingThread(EqualizerViewModel* model, QObject *parent = nullptr);
    ~AudioProcessingThread() override;

    void startAudio();
    void stopAudio();
    bool isRunning() const;

signals:
    void audioStarted();
    void audioStopped();
    void errorOccurred(const QString& error);

protected:
    void run() override;

private slots:
    void onModelBandGainChanged(int band, double gain);
    void onModelAllGainsChanged(const QVector<double>& gains);

private:
    EqualizerViewModel* m_model;
    EqualizerEngine* m_equalizer;
    AudioProcessor* m_audioProcessor;
    QMutex m_mutex;
    std::atomic_bool m_shouldStop;
};

#endif // AUDIOPROCESSORTHREAD_H
