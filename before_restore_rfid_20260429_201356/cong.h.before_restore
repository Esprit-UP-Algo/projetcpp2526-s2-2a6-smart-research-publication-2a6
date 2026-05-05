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
    void showResponse(const QString& html);
    void hideResponse();

protected:
    void mousePressEvent(QMouseEvent*)   override;
    void mouseMoveEvent(QMouseEvent*)    override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*)      override;
    void closeEvent(QCloseEvent* event)  override;

private:
    QMediaPlayer*  m_player;
    QLabel*        m_videoBg;
    QWidget*       m_overlay;
    QTextBrowser*  m_textBrowser;
    QLabel*        m_statusLbl;
    QPushButton*   m_speakBtn;
    QTextToSpeech* m_tts = nullptr;
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

signals:
    void slotClicked(int shelf, int slot);

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

private:
    void loadFreezers();
    void loadFreezer(const QString& cong);
    void rebuildSampleList(const QString& filter = QString());
    void showDetails(int shelf, int slot);
    void clearDetails();
    void callGroq(const QString& msg, const QString& ctx);

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
};

#endif // CONG_H
