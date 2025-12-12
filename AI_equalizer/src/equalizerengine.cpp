#include "equalizerengine.h"

EqualizerEngine::EqualizerEngine(QObject *parent)
    : QObject(parent), m_sampleRate(48000.0)
{
    m_bandGains.resize(NUM_BANDS);
    m_filtersLeft.resize(NUM_BANDS);
    m_filtersRight.resize(NUM_BANDS);
    
    // Initialize all gains to 0 dB (no change)
    for (int i = 0; i < NUM_BANDS; ++i) {
        m_bandGains[i] = 0.0;
    }
    
    updateFilters();
}

void EqualizerEngine::setSampleRate(double rate)
{
    m_sampleRate = rate;
    updateFilters();
}

void EqualizerEngine::setBandGain(int band, double gainDB)
{
    if (band >= 0 && band < NUM_BANDS) {
        m_bandGains[band] = qBound(-30.0, gainDB, 30.0);
        updateFilters();
        emit bandGainChanged(band, m_bandGains[band]);
    }
}

double EqualizerEngine::getBandGain(int band) const
{
    if (band >= 0 && band < NUM_BANDS) {
        return m_bandGains[band];
    }
    return 0.0;
}

void EqualizerEngine::setAllGains(const QVector<double>& gains)
{
    if (gains.size() == NUM_BANDS) {
        m_bandGains = gains;
        updateFilters();
        for (int i = 0; i < NUM_BANDS; ++i) {
            emit bandGainChanged(i, m_bandGains[i]);
        }
    }
}

QVector<double> EqualizerEngine::getAllGains() const
{
    return m_bandGains;
}

void EqualizerEngine::processBuffer(float* buffer, int frameCount, int channels)
{
    if (channels == 2) {
        // Stereo processing
        for (int frame = 0; frame < frameCount; ++frame) {
            float left = buffer[frame * 2];
            float right = buffer[frame * 2 + 1];
            
            // Process through all bands (skip bands with near-zero gain)
            for (int band = 0; band < NUM_BANDS; ++band) {
                if (std::abs(m_bandGains[band]) > 0.01) {  // Only process if gain > 0.01 dB
                    left = m_filtersLeft[band].process(left);
                    right = m_filtersRight[band].process(right);
                }
            }
            
            buffer[frame * 2] = left;
            buffer[frame * 2 + 1] = right;
        }
    } else if (channels == 1) {
        // Mono processing
        for (int frame = 0; frame < frameCount; ++frame) {
            float sample = buffer[frame];
            
            for (int band = 0; band < NUM_BANDS; ++band) {
                if (std::abs(m_bandGains[band]) > 0.01) {  // Only process if gain > 0.01 dB
                    sample = m_filtersLeft[band].process(sample);
                }
            }
            
            buffer[frame] = sample;
        }
    }
}

void EqualizerEngine::reset()
{
    for (int i = 0; i < NUM_BANDS; ++i) {
        m_filtersLeft[i].reset();
        m_filtersRight[i].reset();
    }
}

void EqualizerEngine::updateFilters()
{
    for (int i = 0; i < NUM_BANDS; ++i) {
        double frequency = BAND_FREQUENCIES[i];
        double gain = m_bandGains[i];
        double Q = 1.0; // Bandwidth
        
        m_filtersLeft[i].setPeakingEQ(frequency, m_sampleRate, gain, Q);
        m_filtersRight[i].setPeakingEQ(frequency, m_sampleRate, gain, Q);
    }
}
