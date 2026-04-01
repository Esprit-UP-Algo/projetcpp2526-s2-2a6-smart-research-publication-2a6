#ifndef VOICECOMMANDE_H
#define VOICECOMMANDE_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QAudioSource>
#include <QAudioFormat>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QTimer>
#include <QTextToSpeech>
#include <QMouseEvent>
#include <QVariantMap>

class VoiceCommand : public QWidget
{
    Q_OBJECT
public:
    explicit VoiceCommand(QWidget* parent = nullptr);
    ~VoiceCommand();
    void positionBottomRight();

    // Call this whenever the active page/module changes so the LLM gets context
    void setCurrentContext(int pageIdx, const QString& pageName);

signals:
    void commandExecute(const QString& action, const QString& module, const QVariantMap& params);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void toggleRecording();
    void onNetworkReply(QNetworkReply* reply);

private:
    void startRecording();
    void stopRecordingAndProcess();
    void callGroqStt(const QByteArray& pcmData);
    QByteArray buildWav(const QByteArray& pcm, int sampleRate, int channels) const;
    void setStatus(const QString& text, bool isError = false);
    void speak(const QString& text);

    static QByteArray le32(quint32 v);
    static QByteArray le16(quint16 v);

    // Recording
    QAudioSource*  m_audioSource   = nullptr;
    QBuffer*       m_audioBuffer   = nullptr;
    bool           m_isRecording   = false;

    // Network — track in-flight STT reply so we can abort it if needed
    QNetworkAccessManager* m_net;
    QNetworkReply*         m_pendingReply = nullptr;

    QPushButton* m_micBtn;
    QLabel*      m_statusLbl;
    QLabel*      m_transcriptLbl;
    QLabel*      m_actionLbl;
    QWidget*     m_panel;
    QWidget*     m_dragHandle;

    // Pulse animation for recording
    QTimer* m_pulseTimer;
    int     m_pulseAlpha = 0;
    bool    m_pulseUp    = true;

    // Drag
    QPoint m_dragPos;
    bool   m_dragging = false;

    // TTS feedback
    QTextToSpeech* m_tts = nullptr;

    // Context
    int     m_contextPage = -1;
    QString m_contextName;
};

#endif // VOICECOMMANDE_H
