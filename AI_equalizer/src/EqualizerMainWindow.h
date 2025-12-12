#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QVector>
#include <QTcpSocket>
#include <QTcpServer>
#include <memory>
#include "EqualizerViewModel.h"
#include "AudioProcessingThread.h"
#include "PresetModel.h"
#include "ChatView.h"

namespace Ui {
class EqualizerMainWindow;
}

class EqualizerMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    EqualizerMainWindow(QWidget *parent = nullptr);
    ~EqualizerMainWindow();

private slots:
    void onBandSliderChanged(int value);
    void onPresetChanged(int index);
    void onStartStopClicked();
    void onResetClicked();
    void onChatMessage(const QString& message);
    void onChatResponse();
    void onChatError();
    void onModelBandGainChanged(int band, double gain);
    void onModelAllGainsChanged(const QVector<double>& gains);
    void onAudioStarted();
    void onAudioStopped();
    void onAudioError(const QString& error);
    
private:
    // Qt Designer UI
    std::unique_ptr<Ui::EqualizerMainWindow> ui;
    
    // MVVM Components
    EqualizerViewModel* m_model;
    AudioProcessingThread* m_audioThread;
    PresetModel* m_presetManager;
    
    // UI components
    QVector<QSlider*> m_bandSliders;
    QVector<QLabel*> m_bandLabels;
    QVector<QLabel*> m_bandValueLabels;
    
    // Chat socket
    QTcpSocket* m_chatSocket;
    QString m_currentChatMessage;
    
    // Simple IPC server to receive JSON band gains
    QTcpServer* m_ipcServer;
    void startIpcServer(quint16 port = 5560);
    void onIpcNewConnection();
    void onIpcReadyRead();
    
    void setupUI();
    void createEqualizerControls(QWidget* container);
    void updateSliders(const QVector<double>& gains);
    void updateBandLabel(int band);
    void connectChatAgent();
};

#endif // MAINWINDOW_H
