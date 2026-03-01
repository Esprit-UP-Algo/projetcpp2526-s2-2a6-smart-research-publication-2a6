#ifndef CONG_H
#define CONG_H

#include <QDialog>
#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkReply>
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

// ── Custom-painted freezer visualization ─────────────────────
class FreezerWidget : public QWidget
{
    Q_OBJECT
public:
    static const int N_SHELVES = 5;
    static const int N_SLOTS   = 8;

    struct Slot {
        bool    occupied  = false;
        QString reference;
        QString danger;
    };

    explicit FreezerWidget(QWidget* parent = nullptr);

    // [0] = top shelf (Étage 5), [4] = bottom shelf (Étage 1)
    void setData(const QVector<QVector<Slot>>& shelves);
    void selectSlot(int shelf, int slot);   // -1,-1 clears
    void clearSelection();

    QSize sizeHint() const override { return {500, 420}; }

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
};

// ── Main dialog ───────────────────────────────────────────────
class CongelateurDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CongelateurDialog(QWidget* parent = nullptr);
    void refresh();

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
    QLineEdit*   m_searchEdit;
    QComboBox*   m_searchFilter;
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

    // Right — AI
    QLineEdit*   m_aiInput;
    QPushButton* m_aiBtn;
    QLabel*      m_aiResp;

    QNetworkAccessManager* m_net;
    CrudeBioSimple*        m_crud;

    QString     m_currentCong;
    QString     m_lastContext;

    QStringList                       m_shelfKeys;
    QVector<QVector<SlotInfo>>        m_slotData;
};

#endif // CONG_H
