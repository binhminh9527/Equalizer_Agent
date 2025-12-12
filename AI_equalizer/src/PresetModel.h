#ifndef PRESETMODEL_H
#define PRESETMODEL_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>

struct EQPreset {
    QString name;
    QVector<double> bandGains; // 10 bands
    
    EQPreset() : bandGains(10, 0.0) {}
    EQPreset(const QString& n, const QVector<double>& gains) 
        : name(n), bandGains(gains) {}
};

class PresetModel : public QObject
{
    Q_OBJECT

public:
    explicit PresetModel(QObject *parent = nullptr);
    
    QStringList getPresetNames() const;
    EQPreset getPreset(const QString& name) const;
    QVector<double> getPresetGains(int index) const;
    bool hasPreset(const QString& name) const;
    void savePreset(const QString& name, const QVector<double>& gains);
    
private:
    QMap<QString, EQPreset> m_presets;
    
    void initializeDefaultPresets();
};

#endif // PRESETMODEL_H
