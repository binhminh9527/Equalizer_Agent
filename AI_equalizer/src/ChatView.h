#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

class ChatView : public QWidget
{
    Q_OBJECT

public:
    explicit ChatView(QWidget *parent = nullptr);
    
    void addUserMessage(const QString& message);
    void addAIMessage(const QString& message);
    void addSystemMessage(const QString& message);
    
signals:
    void messageSent(const QString& message);
    
private slots:
    void onSendClicked();
    
private:
    QTextEdit* m_chatDisplay;
    QLineEdit* m_inputField;
    QPushButton* m_sendButton;
    
    void setupUI();
    void appendMessage(const QString& sender, const QString& message, const QString& color);
};

#endif // CHATVIEW_H
