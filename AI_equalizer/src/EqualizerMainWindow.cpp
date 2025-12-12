#include "EqualizerMainWindow.h"
#include "ui_EqualizerMainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

EqualizerMainWindow::EqualizerMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(std::make_unique<Ui::EqualizerMainWindow>())
{
    // Setup UI from designer file
    ui->setupUi(this);
    
    // Initialize MVVM components
    m_model = new EqualizerViewModel(this);
    m_audioThread = new AudioProcessingThread(m_model, this);
    m_presetManager = new PresetModel(this);
    
    // Connect model signals to view updates
    connect(m_model, &EqualizerViewModel::bandGainChanged,
            this, &EqualizerMainWindow::onModelBandGainChanged);
    connect(m_model, &EqualizerViewModel::allGainsChanged,
            this, &EqualizerMainWindow::onModelAllGainsChanged);
    
    // Connect audio thread signals
    connect(m_audioThread, &AudioProcessingThread::audioStarted,
            this, &EqualizerMainWindow::onAudioStarted);
    connect(m_audioThread, &AudioProcessingThread::audioStopped,
            this, &EqualizerMainWindow::onAudioStopped);
    connect(m_audioThread, &AudioProcessingThread::errorOccurred,
            this, &EqualizerMainWindow::onAudioError);
    
    // Setup chat socket
    m_chatSocket = new QTcpSocket(this);
    connect(m_chatSocket, &QTcpSocket::readyRead, this, &EqualizerMainWindow::onChatResponse);
    connect(m_chatSocket, static_cast<void(QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::errorOccurred),
            this, &EqualizerMainWindow::onChatError);
    
    // Setup additional UI elements (programmatically create sliders)
    setupUI();
    
    // Connect to chat agent
    connectChatAgent();

    // Start IPC server for JSON gains
    startIpcServer(5560);
}

EqualizerMainWindow::~EqualizerMainWindow()
{
    if (m_audioThread->isRunning()) {
        m_audioThread->stopAudio();
    }
}

void EqualizerMainWindow::setupUI()
{
    // Connect UI buttons from designer
    connect(ui->startStopButton, &QPushButton::clicked,
            this, &EqualizerMainWindow::onStartStopClicked);
    connect(ui->resetButton, &QPushButton::clicked,
            this, &EqualizerMainWindow::onResetClicked);
    connect(ui->presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EqualizerMainWindow::onPresetChanged);
    
    // Connect chat view
    connect(ui->chatWidget, &ChatView::messageSent,
            this, &EqualizerMainWindow::onChatMessage);
    
    // Populate preset combo box
    ui->presetCombo->addItems(m_presetManager->getPresetNames());
    
    // Create the 10 EQ sliders programmatically
    createEqualizerControls(ui->eqGroup);
}

void EqualizerMainWindow::createEqualizerControls(QWidget* container)
{
    // Delete existing layout if present
    if (container->layout()) {
        delete container->layout();
    }
    
    QHBoxLayout* mainLayout = new QHBoxLayout(container);
    mainLayout->setSpacing(10);
    
    const QStringList bandNames = {
        "31Hz", "62Hz", "125Hz", "250Hz", "500Hz",
        "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"
    };
    
    for (int i = 0; i < 10; i++) {
        QWidget* bandWidget = new QWidget();
        QVBoxLayout* bandLayout = new QVBoxLayout(bandWidget);
        bandLayout->setSpacing(2);
        bandLayout->setContentsMargins(0, 0, 0, 0);
        
        // Frequency label
        QLabel* freqLabel = new QLabel(bandNames[i]);
        freqLabel->setAlignment(Qt::AlignCenter);
        m_bandLabels.append(freqLabel);
        bandLayout->addWidget(freqLabel, 0, Qt::AlignCenter);
        
        // Slider
        QSlider* slider = new QSlider(Qt::Vertical);
        slider->setMinimum(-30);
        slider->setMaximum(30);
        slider->setValue(0);
        slider->setTickPosition(QSlider::TicksLeft);
        slider->setTickInterval(6);
        slider->setMinimumHeight(200);
        m_bandSliders.append(slider);
        bandLayout->addWidget(slider, 1);
        
        // Value label
        QLabel* valueLabel = new QLabel("0dB");
        valueLabel->setAlignment(Qt::AlignCenter);
        m_bandValueLabels.append(valueLabel);
        bandLayout->addWidget(valueLabel, 0, Qt::AlignCenter);
        
        mainLayout->addWidget(bandWidget);
        
        connect(slider, &QSlider::valueChanged,
                this, &EqualizerMainWindow::onBandSliderChanged);
    }
}

void EqualizerMainWindow::onBandSliderChanged(int value)
{
    QSlider* slider = qobject_cast<QSlider*>(sender());
    if (!slider) return;
    
    int bandIndex = m_bandSliders.indexOf(slider);
    if (bandIndex >= 0) {
        m_model->setBandGain(bandIndex, static_cast<double>(value));
        updateBandLabel(bandIndex);
    }
}

void EqualizerMainWindow::updateBandLabel(int band)
{
    if (band >= 0 && band < m_bandValueLabels.size()) {
        double gain = m_model->getBandGain(band);
        m_bandValueLabels[band]->setText(QString("%1dB").arg(gain, 0, 'f', 1));
    }
}

void EqualizerMainWindow::onPresetChanged(int index)
{
    if (index >= 0 && index < m_presetManager->getPresetNames().size()) {
        QVector<double> gains = m_presetManager->getPresetGains(index);
        m_model->setAllBandGains(gains);
        updateSliders(gains);
    }
}

void EqualizerMainWindow::updateSliders(const QVector<double>& gains)
{
    for (int i = 0; i < gains.size() && i < m_bandSliders.size(); ++i) {
        m_bandSliders[i]->blockSignals(true);
        m_bandSliders[i]->setValue(static_cast<int>(gains[i]));
        m_bandSliders[i]->blockSignals(false);
        updateBandLabel(i);
    }
}

void EqualizerMainWindow::onStartStopClicked()
{
    if (m_audioThread->isRunning()) {
        m_audioThread->stopAudio();
        ui->startStopButton->setText("Start Audio");
    } else {
        m_audioThread->startAudio();
        ui->startStopButton->setText("Stop Audio");
    }
}

void EqualizerMainWindow::onResetClicked()
{
    QVector<double> flatGains(10, 0.0);
    m_model->setAllBandGains(flatGains);
    ui->presetCombo->setCurrentIndex(0);  // Set to "Flat" preset
}

void EqualizerMainWindow::connectChatAgent()
{
    m_chatSocket->connectToHost("localhost", 5555);
    if (m_chatSocket->waitForConnected(2000)) {
        qDebug() << "✓ Connected to Chat Agent";
        ui->chatWidget->addSystemMessage("Connected to chat agent");
    } else {
        qDebug() << "✗ Failed to connect to Chat Agent (is it running?)";
        ui->chatWidget->addSystemMessage("Chat agent not available. Start it with: python3 agent/chat_agent.py");
    }
}

void EqualizerMainWindow::onChatMessage(const QString& message)
{
    // Check connection state and attempt to reconnect if needed
    if (m_chatSocket->state() != QTcpSocket::ConnectedState) {
        ui->chatWidget->addSystemMessage("Reconnecting to chat agent...");
        
        // Attempt to reconnect
        m_chatSocket->abort(); // Close any existing connection
        m_chatSocket->connectToHost("localhost", 5555);
        
        if (!m_chatSocket->waitForConnected(2000)) {
            ui->chatWidget->addSystemMessage("Failed to reconnect. Is the chat agent running?");
            return;
        }
        
        ui->chatWidget->addSystemMessage("Reconnected!");
    }
    
    m_currentChatMessage = message;
    m_chatSocket->write(message.toUtf8());
    qDebug() << "Sent to agent:" << message;
}

void EqualizerMainWindow::onChatResponse()
{
    QByteArray data = m_chatSocket->readAll();
    qDebug() << "Response:" << data;
    
    // Parse JSON response
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        QString response = obj["response"].toString();
        ui->chatWidget->addAIMessage(response);
    } else {
        ui->chatWidget->addAIMessage(QString::fromUtf8(data));
    }
}

void EqualizerMainWindow::onChatError()
{
    qDebug() << "Chat socket error:" << m_chatSocket->errorString();
    ui->chatWidget->addSystemMessage("Connection error: " + m_chatSocket->errorString());
}

void EqualizerMainWindow::onModelBandGainChanged(int band, double gain)
{
    // Update UI when model changes
    updateBandLabel(band);
}

void EqualizerMainWindow::onModelAllGainsChanged(const QVector<double>& gains)
{
    // Update all sliders when model changes
    updateSliders(gains);
}

void EqualizerMainWindow::onAudioStarted()
{
    qDebug() << "Audio processing started";
    ui->startStopButton->setText("Stop Audio");
}

void EqualizerMainWindow::onAudioStopped()
{
    qDebug() << "Audio processing stopped";
    ui->startStopButton->setText("Start Audio");
}

void EqualizerMainWindow::onAudioError(const QString& error)
{
    ui->startStopButton->setText("Start Audio");
    QMessageBox::warning(this, "Audio Error", error);
}

// ========== IPC server: receive JSON band gains ==========
void EqualizerMainWindow::startIpcServer(quint16 port)
{
    m_ipcServer = new QTcpServer(this);
    connect(m_ipcServer, &QTcpServer::newConnection, this, &EqualizerMainWindow::onIpcNewConnection);
    if (!m_ipcServer->listen(QHostAddress::LocalHost, port)) {
        qWarning() << "IPC listen failed on" << port << ":" << m_ipcServer->errorString();
    } else {
        qDebug() << "IPC listening on" << m_ipcServer->serverAddress().toString() << port;
    }
}

void EqualizerMainWindow::onIpcNewConnection()
{
    while (m_ipcServer->hasPendingConnections()) {
        QTcpSocket* s = m_ipcServer->nextPendingConnection();
        connect(s, &QTcpSocket::readyRead, this, &EqualizerMainWindow::onIpcReadyRead);
        connect(s, &QTcpSocket::disconnected, s, &QTcpSocket::deleteLater);
    }
}

void EqualizerMainWindow::onIpcReadyRead()
{
    QTcpSocket* s = qobject_cast<QTcpSocket*>(sender());
    if (!s) return;
    const QByteArray data = s->readAll();
    const QString json = QString::fromUtf8(data).trimmed();
    bool ok = m_model->setBandGainsJson(json);
    s->write(ok ? QByteArray("OK\n") : QByteArray("ERROR\n"));
    s->flush();
}
