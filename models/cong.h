#ifndef CONG_H
#define CONG_H

#include <QDialog>
#include <QWidget>
#include <QCloseEvent>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QTextBrowser>
#include <QTextToSpeech>
#include <QSerialPort>
#include <QMap>
#include <QDateTime>
#include "crudebiosimple.h"

// ── Per-slot data (full sample info) ─────────────────────────
struct SlotInfo {
    QString reference;
    QString type;
    QString organisme;
    QString emplacement;
    QString temperature;
    QString danger;
    int     quantite       = 0;
    QString dateCollecte;
    QString dateExpiration;
    QString etage;
};

// ── Floating draggable AI response bubble ─────────────────────
class AiBubble : public QFrame
{
    Q_OBJECT
public:
    explicit AiBubble(QWidget* parent = nullptr);
    ~AiBubble();
    void showResponse(const QString& html);
    void hideResponse();

protected:
    void mousePressEvent(QMouseEvent*)   override;
    void mouseMoveEvent(QMouseEvent*)    override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*)      override;
    void closeEvent(QCloseEvent* event)  override;

private:
    void          resetSpeechUi();
    void          requestSpeechStop();
    QMediaPlayer*  m_player;
    QLabel*        m_videoBg;
    QWidget*       m_overlay;
    QTextBrowser*  m_textBrowser;
    QLabel*        m_statusLbl;
    QPushButton*   m_speakBtn;
    QTextToSpeech* m_tts = nullptr;
    bool           m_speechStopRequested = false;
    QPoint         m_dragPos;
    bool           m_dragging = false;
};

// ── Custom-painted freezer visualization ─────────────────────
class FreezerWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float doorOpen READ doorOpen WRITE setDoorOpen)

public:
    static const int N_SHELVES = 5;
    static const int N_SLOTS   = 8;

    struct Slot {
        bool    occupied  = false;
        QString reference;
        QString danger;
    };

    explicit FreezerWidget(QWidget* parent = nullptr);

    void setData(const QVector<QVector<Slot>>& shelves);
    void selectSlot(int shelf, int slot);
    void clearSelection();

    void setFreezerName(const QString& name) { m_freezerName = name; update(); }

    float doorOpen() const { return m_doorOpen; }
    void  setDoorOpen(float v) { m_doorOpen = qBound(0.0f, v, 1.0f); update(); }

    QSize sizeHint()        const override { return {400, 620}; }
    QSize minimumSizeHint() const override { return {320, 500}; }

public slots:
    void openDoor();
    void closeDoor();
    // Set door open/closed state, optionally animated (called by Arduino SERVO events)
    void setDoorState(bool open, bool animate = true);
    // Update the live temperature shown on the door LCD (called by Arduino TEMP events)
    void setCurrentTemperature(double celsius);

signals:
    void slotClicked(int shelf, int slot);
    // Emitted when the user clicks the Ouvrir/Fermer button on the door
    void openRequested();
    void closeRequested();

protected:
    void paintEvent(QPaintEvent*)      override;
    void mousePressEvent(QMouseEvent*) override;

private:
    QRectF bodyRect()              const;
    QRectF innerRect()             const;
    QRectF shelfBand(int row)      const;
    QRectF slotRectF(int row, int col) const;
    void   drawPin(QPainter& p, QRectF sr) const;

    QVector<QVector<Slot>> m_data;
    int m_selShelf = -1;
    int m_selSlot  = -1;

    // Door animation
    float               m_doorOpen    = 0.0f;
    QString             m_freezerName;
    QPropertyAnimation* m_doorAnim;

    // Clock & logo
    QTimer*  m_clockTimer;
    bool     m_colonVisible = true;
    QPixmap  m_logoPixmap;

    // Live temperature from Arduino DHT11
    double  m_currentTemp = -80.0;
    bool    m_hasTemp     = false;

    // Door button hit zones (set each paintEvent)
    QRectF m_doorOpenBtnRect;
    QRectF m_doorCloseBtnRect;
};

// ── Main dialog ───────────────────────────────────────────────
class CongelateurDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CongelateurDialog(QWidget* parent = nullptr);
    void refresh();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onFreezerClicked(QListWidgetItem* item);
    void onSlotClicked(int shelf, int slot);
    void onSampleListClicked(QListWidgetItem* item);
    void onSearchChanged(const QString& text);
    void onAiSearch();
    void onAiReply(QNetworkReply* reply);
    // Arduino serial
    void onSerialReadyRead();
    void onManualOpenClicked();
    void onManualCloseClicked();

private:
    void loadFreezers();
    void loadFreezer(const QString& cong);
    void rebuildSampleList(const QString& filter = QString());
    void showDetails(int shelf, int slot);
    void clearDetails();
    void callGroq(const QString& msg, const QString& ctx);

    // ── DB init ──────────────────────────────────────────────
    void setupTables();

    // ── Arduino / serial ──────────────────────────────────────
    void setupArduino();
    void parseSerialLine(const QString& line);
    void handleTempUpdate(int doorNum, double temp, double hum, double thr, bool ok);
    void handleServoStatus(int doorNum, const QString& status);
    void handleRfidScan(const QString& uid);
    void handleAccessAuthorized(int doorNum);
    void handleAccessDenied(int doorNum, const QString& reason, double temp);
    void insertAlert(int frigoId, double value, int badgeId);
    void logAccess(int badgeId, int frigoId, const QString& action);
    void logAccess(int badgeId, const QString& action);
    void sendArduinoCommand(const QString& cmd);
    void startAutoCloseTimer(int doorNum);
    void cancelAutoCloseTimer(int doorNum);
    int  currentDisplayedFrigoId() const;
    int  currentDisplayedDoorNum() const;
    int  frigoIdByName(const QString& name) const;
    int  frigoIdByDoorNum(int doorNum) const;
    int  doorNumByFrigoId(int frigoId) const;
    bool frigoStatusIsOpen(int frigoId) const;
    int  badgeIdByUid(const QString& uid) const;

    // Left panel
    QListWidget* m_freezerList;
    QLineEdit*   m_searchEdit   = nullptr;
    QComboBox*   m_searchFilter = nullptr;
    QListWidget* m_sampleList;

    // Center
    FreezerWidget* m_freezerWidget;
    QLabel*        m_statusBar;

    // Right — details
    QLabel* m_pinIcon;
    QLabel* m_detId;
    QLabel* m_detType;
    QLabel* m_detOrg;
    QLabel* m_detEtage;
    QLabel* m_detSlot;
    QLabel* m_detTemp;
    QLabel* m_detDanger;
    QLabel* m_detDateCol;
    QLabel* m_detDateExp;

    // Right — AI input
    QLineEdit*   m_aiInput;
    QPushButton* m_aiBtn;
    QLabel*      m_aiResp;

    // Floating AI response bubble
    AiBubble* m_aiBubble = nullptr;

    QNetworkAccessManager* m_net;
    CrudeBioSimple*        m_crud;
    QPixmap                m_bgPixmap;

    QString     m_currentCong;
    QString     m_lastContext;

    QStringList                       m_shelfKeys;
    QVector<QVector<SlotInfo>>        m_slotData;

    // ── Arduino serial state ──────────────────────────────────
    QSerialPort*          m_arduino        = nullptr;
    QByteArray            m_serialBuffer;
    QMap<int, double>     m_currentTemps;      // doorNum → °C
    QMap<int, double>     m_currentHumidities; // doorNum → %
    QMap<int, QDateTime>  m_lastAlertTime;     // debounce per door
    QMap<int, QString>    m_pendingScanUid;    // doorNum → RFID UID
    QMap<int, bool>       m_doorOpen;          // doorNum → porte physiquement ouverte
    QMap<int, QTimer*>    m_autoCloseTimers;   // doorNum → fermeture auto 10 s
};

#endif // CONG_H
