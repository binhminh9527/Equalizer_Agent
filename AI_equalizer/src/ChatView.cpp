#include "ChatView.h"
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDateTime>

ChatView::ChatView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ChatView::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Chat display area
    m_chatDisplay = new QTextEdit(this);
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setMinimumHeight(200);
    
    // Input area
    QHBoxLayout* inputLayout = new QHBoxLayout();
    
    m_inputField = new QLineEdit(this);
    m_inputField->setPlaceholderText("Ask AI to adjust the equalizer...");
    
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setMaximumWidth(80);
    
    inputLayout->addWidget(m_inputField);
    inputLayout->addWidget(m_sendButton);
    
    mainLayout->addWidget(m_chatDisplay, 1);
    mainLayout->addLayout(inputLayout);
    
    // Connections
    connect(m_sendButton, &QPushButton::clicked, this, &ChatView::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed, this, &ChatView::onSendClicked);
    
    // Initial message
    addSystemMessage("Welcome! I'm your AI equalizer assistant. Tell me how you'd like to adjust the sound.");
}

void ChatView::onSendClicked()
{
    QString message = m_inputField->text().trimmed();
    if (!message.isEmpty()) {
        addUserMessage(message);
        emit messageSent(message);
        m_inputField->clear();
    }
}

void ChatView::addUserMessage(const QString& message)
{
    appendMessage("You", message, "#2196F3");
}

void ChatView::addAIMessage(const QString& message)
{
    // Remove the EQ_ADJUSTMENT line from display
    QString displayMessage = message;
    int eqPos = displayMessage.indexOf("EQ_ADJUSTMENT:");
    if (eqPos != -1) {
        int lineEnd = displayMessage.indexOf('\n', eqPos);
        if (lineEnd != -1) {
            displayMessage.remove(eqPos, lineEnd - eqPos + 1);
        } else {
            displayMessage = displayMessage.left(eqPos);
        }
    }
    
    appendMessage("AI", displayMessage.trimmed(), "#4CAF50");
}

void ChatView::addSystemMessage(const QString& message)
{
    appendMessage("System", message, "#FF9800");
}

void ChatView::appendMessage(const QString& sender, const QString& message, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    
    QString html = QString("<div style='margin-bottom: 10px;'>"
                          "<span style='color: %1; font-weight: bold;'>%2</span> "
                          "<span style='color: gray; font-size: 10px;'>%3</span><br>"
                          "<span style='margin-left: 10px;'>%4</span>"
                          "</div>")
                       .arg(color, sender, timestamp, message.toHtmlEscaped());
    
    m_chatDisplay->append(html);
    
    // Auto-scroll to bottom
    QScrollBar* scrollBar = m_chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}
