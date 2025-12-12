#include "EqualizerViewModel.h"
#include "equalizerengine.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

EqualizerViewModel::EqualizerViewModel(QObject *parent)
    : QObject(parent), m_audioRunning(false)
{
    m_bandGains.resize(EqualizerEngine::NUM_BANDS);
    for (int i = 0; i < EqualizerEngine::NUM_BANDS; ++i) {
        m_bandGains[i] = 0.0;
    }
}

QVector<double> EqualizerViewModel::getBandGains() const
{
    QMutexLocker locker(&m_mutex);
    return m_bandGains;
}

double EqualizerViewModel::getBandGain(int band) const
{
    QMutexLocker locker(&m_mutex);
    if (band >= 0 && band < m_bandGains.size()) {
        return m_bandGains[band];
    }
    return 0.0;
}

void EqualizerViewModel::setBandGain(int band, double gain)
{
    {
        QMutexLocker locker(&m_mutex);
        if (band >= 0 && band < m_bandGains.size()) {
            m_bandGains[band] = gain;
        }
    }
    emit bandGainChanged(band, gain);
}

void EqualizerViewModel::setAllBandGains(const QVector<double>& gains)
{
    {
        QMutexLocker locker(&m_mutex);
        if (gains.size() == m_bandGains.size()) {
            m_bandGains = gains;
        }
    }
    emit allGainsChanged(gains);
}

bool EqualizerViewModel::setBandGainsJson(const QString& jsonArrayString)
{
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonArrayString.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        return false;
    }
    QJsonArray arr = doc.array();
    if (arr.size() != m_bandGains.size()) {
        return false;
    }
    QVector<double> gains;
    gains.reserve(arr.size());
    for (int i = 0; i < arr.size(); ++i) {
        const QJsonValue& v = arr.at(i);
        if (!v.isDouble()) {
            return false;
        }
        gains.push_back(v.toDouble());
    }
    setAllBandGains(gains);
    return true;
}

bool EqualizerViewModel::isAudioRunning() const
{
    QMutexLocker locker(&m_mutex);
    return m_audioRunning;
}

void EqualizerViewModel::setAudioRunning(bool running)
{
    {
        QMutexLocker locker(&m_mutex);
        m_audioRunning = running;
    }
    emit audioRunningChanged(running);
}
