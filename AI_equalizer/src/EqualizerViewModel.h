#ifndef EQUALIZERVIEWMODEL_H
#define EQUALIZERVIEWMODEL_H

#include <QObject>
#include <QVector>
#include <QMutex>

// Model: Holds the data state
class EqualizerViewModel : public QObject
{
    Q_OBJECT

public:
    explicit EqualizerViewModel(QObject *parent = nullptr);

    QVector<double> getBandGains() const;
    double getBandGain(int band) const;
    void setBandGain(int band, double gain);
    void setAllBandGains(const QVector<double>& gains);
    // Accept a JSON array of doubles (size must match number of bands)
    Q_INVOKABLE bool setBandGainsJson(const QString& jsonArrayString);

    bool isAudioRunning() const;
    void setAudioRunning(bool running);

signals:
    void bandGainChanged(int band, double gain);
    void allGainsChanged(const QVector<double>& gains);
    void audioRunningChanged(bool running);

private:
    mutable QMutex m_mutex;
    QVector<double> m_bandGains;
    bool m_audioRunning;
};

#endif // EQUALIZERVIEWMODEL_H
