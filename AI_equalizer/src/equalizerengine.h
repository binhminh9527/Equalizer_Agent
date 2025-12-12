#ifndef EQUALIZERENGINE_H
#define EQUALIZERENGINE_H

#include <QObject>
#include <QVector>
#include <cmath>

// Biquad filter implementation for each band
class BiquadFilter {
public:
    BiquadFilter() : b0(1.0), b1(0.0), b2(0.0), a1(0.0), a2(0.0),
                     x1(0.0), x2(0.0), y1(0.0), y2(0.0) {}
    
    void setPeakingEQ(double frequency, double sampleRate, double gainDB, double Q = 1.0) {
        double A = std::pow(10.0, gainDB / 40.0);  // A = sqrt(10^(dB/20)) = 10^(dB/40)
        double omega = 2.0 * M_PI * frequency / sampleRate;
        double sn = std::sin(omega);
        double cs = std::cos(omega);
        double alpha = sn / (2.0 * Q);
        
        // Audio EQ Cookbook peaking EQ with constant 0 dB peak gain
        double b0_temp = 1.0 + alpha * A;
        double b1_temp = -2.0 * cs;
        double b2_temp = 1.0 - alpha * A;
        double a0_temp = 1.0 + alpha / A;
        double a1_temp = -2.0 * cs;
        double a2_temp = 1.0 - alpha / A;
        
        // Normalize by a0
        b0 = b0_temp / a0_temp;
        b1 = b1_temp / a0_temp;
        b2 = b2_temp / a0_temp;
        a1 = a1_temp / a0_temp;
        a2 = a2_temp / a0_temp;
    }
    
    float process(float input) {
        // Clamp input to prevent denormal numbers
        if (std::abs(input) < 1e-15f) input = 0.0f;
        
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Prevent denormal numbers in state variables
        if (std::abs(output) < 1e-15f) output = 0.0f;
        
        // Clamp output to valid audio range [-10.0, 10.0] to prevent clipping
        output = std::max(-10.0f, std::min(10.0f, output));
        
        x2 = x1;
        x1 = input;
        y2 = y1;
        y1 = output;
        
        return output;
    }
    
    void reset() {
        x1 = x2 = y1 = y2 = 0.0;
    }
    
private:
    double b0, b1, b2, a1, a2;
    double x1, x2, y1, y2;
};

class EqualizerEngine : public QObject
{
    Q_OBJECT

public:
    explicit EqualizerEngine(QObject *parent = nullptr);
    
    // 10 frequency bands (Hz)
    static constexpr int NUM_BANDS = 10;
    static constexpr double BAND_FREQUENCIES[NUM_BANDS] = {
        31.25, 62.5, 125, 250, 500, 1000, 2000, 4000, 8000, 16000
    };
    
    void setSampleRate(double rate);
    void setBandGain(int band, double gainDB);
    double getBandGain(int band) const;
    void setAllGains(const QVector<double>& gains);
    QVector<double> getAllGains() const;
    
    void processBuffer(float* buffer, int frameCount, int channels);
    void reset();
    
signals:
    void bandGainChanged(int band, double gain);
    
private:
    double m_sampleRate;
    QVector<double> m_bandGains;
    QVector<BiquadFilter> m_filtersLeft;
    QVector<BiquadFilter> m_filtersRight;
    
    void updateFilters();
};

#endif // EQUALIZERENGINE_H
