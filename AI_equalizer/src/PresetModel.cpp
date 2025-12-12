#include "PresetModel.h"

PresetModel::PresetModel(QObject *parent)
    : QObject(parent)
{
    initializeDefaultPresets();
}

QStringList PresetModel::getPresetNames() const
{
    return m_presets.keys();
}

EQPreset PresetModel::getPreset(const QString& name) const
{
    return m_presets.value(name, EQPreset());
}

QVector<double> PresetModel::getPresetGains(int index) const
{
    QStringList names = getPresetNames();
    if (index >= 0 && index < names.size()) {
        return m_presets.value(names[index]).bandGains;
    }
    return QVector<double>(10, 0.0);
}

bool PresetModel::hasPreset(const QString& name) const
{
    return m_presets.contains(name);
}

void PresetModel::savePreset(const QString& name, const QVector<double>& gains)
{
    m_presets[name] = EQPreset(name, gains);
}

void PresetModel::initializeDefaultPresets()
{
    // Flat (no change)
    m_presets["Flat"] = EQPreset("Flat", QVector<double>(10, 0.0));
    
    // Rock - Enhanced lows and highs
    m_presets["Rock"] = EQPreset("Rock", {
        5.0,   // 31.25 Hz
        4.0,   // 62.5 Hz
        3.0,   // 125 Hz
        1.0,   // 250 Hz
        -1.0,  // 500 Hz
        -0.5,  // 1 kHz
        1.0,   // 2 kHz
        3.0,   // 4 kHz
        4.0,   // 8 kHz
        5.0    // 16 kHz
    });
    
    // Pop - Emphasis on vocals and bass
    m_presets["Pop"] = EQPreset("Pop", {
        2.0,   // 31.25 Hz
        1.5,   // 62.5 Hz
        0.0,   // 125 Hz
        -1.0,  // 250 Hz
        -2.0,  // 500 Hz
        -1.5,  // 1 kHz
        0.0,   // 2 kHz
        1.5,   // 4 kHz
        2.5,   // 8 kHz
        3.0    // 16 kHz
    });
    
    // Jazz - Mid-focused with smooth response
    m_presets["Jazz"] = EQPreset("Jazz", {
        2.0,   // 31.25 Hz
        1.0,   // 62.5 Hz
        0.0,   // 125 Hz
        1.0,   // 250 Hz
        2.0,   // 500 Hz
        2.0,   // 1 kHz
        1.0,   // 2 kHz
        0.0,   // 4 kHz
        1.0,   // 8 kHz
        2.0    // 16 kHz
    });
    
    // Classical - Natural with enhanced dynamics
    m_presets["Classical"] = EQPreset("Classical", {
        3.0,   // 31.25 Hz
        2.0,   // 62.5 Hz
        0.0,   // 125 Hz
        0.0,   // 250 Hz
        -1.0,  // 500 Hz
        0.0,   // 1 kHz
        0.0,   // 2 kHz
        1.0,   // 4 kHz
        2.0,   // 8 kHz
        3.0    // 16 kHz
    });
    
    // Bass Boost
    m_presets["Bass Boost"] = EQPreset("Bass Boost", {
        8.0,   // 31.25 Hz
        7.0,   // 62.5 Hz
        6.0,   // 125 Hz
        4.0,   // 250 Hz
        2.0,   // 500 Hz
        0.0,   // 1 kHz
        0.0,   // 2 kHz
        0.0,   // 4 kHz
        0.0,   // 8 kHz
        0.0    // 16 kHz
    });
    
    // Treble Boost
    m_presets["Treble Boost"] = EQPreset("Treble Boost", {
        0.0,   // 31.25 Hz
        0.0,   // 62.5 Hz
        0.0,   // 125 Hz
        0.0,   // 250 Hz
        1.0,   // 500 Hz
        2.0,   // 1 kHz
        4.0,   // 2 kHz
        6.0,   // 4 kHz
        7.0,   // 8 kHz
        8.0    // 16 kHz
    });
    
    // Vocal
    m_presets["Vocal"] = EQPreset("Vocal", {
        -2.0,  // 31.25 Hz
        -2.0,  // 62.5 Hz
        -1.0,  // 125 Hz
        1.0,   // 250 Hz
        3.0,   // 500 Hz
        4.0,   // 1 kHz
        4.0,   // 2 kHz
        3.0,   // 4 kHz
        1.0,   // 8 kHz
        0.0    // 16 kHz
    });
    
    // Electronic
    m_presets["Electronic"] = EQPreset("Electronic", {
        6.0,   // 31.25 Hz
        5.0,   // 62.5 Hz
        3.0,   // 125 Hz
        0.0,   // 250 Hz
        -1.0,  // 500 Hz
        0.0,   // 1 kHz
        2.0,   // 2 kHz
        4.0,   // 4 kHz
        5.0,   // 8 kHz
        6.0    // 16 kHz
    });
    
    // Acoustic
    m_presets["Acoustic"] = EQPreset("Acoustic", {
        3.0,   // 31.25 Hz
        2.5,   // 62.5 Hz
        2.0,   // 125 Hz
        1.5,   // 250 Hz
        1.0,   // 500 Hz
        1.5,   // 1 kHz
        2.0,   // 2 kHz
        2.5,   // 4 kHz
        2.0,   // 8 kHz
        1.5    // 16 kHz
    });
}
