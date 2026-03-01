#include "cong.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QSslError>

static const QString GROQ_KEY   = "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";
static const QString GROQ_URL   = "https://api.groq.com/openai/v1/chat/completions";
static const QString GROQ_MODEL = "llama-3.1-8b-instant";

// ─── Emplacement helpers ──────────────────────────────────────
static QString parseCong(const QString& emp) {
    int s = emp.indexOf("Cong:") + 5, e = emp.indexOf("/", s);
    return (s >= 5 && e > s) ? emp.mid(s, e - s) : QString();
}
static QString parseEtag(const QString& emp) {
    int s = emp.indexOf("Etag:") + 5;
    return s >= 5 ? emp.mid(s) : QString();
}
static int etageNum(const QString& etage) {
    QString s = etage;
    if (s.startsWith('E', Qt::CaseInsensitive)) s = s.mid(1);
    bool ok; int n = s.toInt(&ok);
    return ok ? n : 1;
}
static int etageToRow(const QString& etage) {
    return qBound(0, FreezerWidget::N_SHELVES - etageNum(etage), FreezerWidget::N_SHELVES - 1);
}
static QString rowToLabel(int row) {
    return QString("Étage %1").arg(FreezerWidget::N_SHELVES - row);
}

// ═══════════════════════════════════════════════════════════════
// FreezerWidget
// ═══════════════════════════════════════════════════════════════
FreezerWidget::FreezerWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(440, 360);
    m_data.resize(N_SHELVES, QVector<Slot>(N_SLOTS));
}

void FreezerWidget::setData(const QVector<QVector<Slot>>& shelves) {
    m_data = shelves; m_selShelf = m_selSlot = -1; update();
}
void FreezerWidget::selectSlot(int shelf, int slot) {
    m_selShelf = shelf; m_selSlot = slot; update();
}
void FreezerWidget::clearSelection() { selectSlot(-1, -1); }

QRectF FreezerWidget::bodyRect()  const {
    return QRectF(8, 6, width() - 16, height() - 12);
}
QRectF FreezerWidget::innerRect() const {
    QRectF b = bodyRect();
    return QRectF(b.x() + 42, b.y() + 14, b.width() - 50, b.height() - 28);
}
QRectF FreezerWidget::shelfBand(int row) const {
    QRectF ir = innerRect();
    float h = ir.height() / float(N_SHELVES);
    return QRectF(ir.x(), ir.y() + row * h, ir.width(), h);
}
QRectF FreezerWidget::slotRectF(int row, int col) const {
    QRectF band = shelfBand(row);
    float labelW = 72.0f, gap = 4.0f;
    float slotsW = band.width() - labelW - 8.0f;
    float slotW  = (slotsW - gap * (N_SLOTS - 1)) / N_SLOTS;
    float slotH  = band.height() - 20.0f;
    float x = band.x() + labelW + 4.0f + col * (slotW + gap);
    float y = band.y() + 10.0f;
    return QRectF(x, y, slotW, slotH);
}

void FreezerWidget::drawPin(QPainter& p, QRectF sr) const {
    float cx = sr.center().x(), tipY = sr.top() - 3.0f;
    float r  = 10.0f, cxF = cx, cyF = tipY - r * 2.0f;
    QPainterPath path;
    path.moveTo(cxF, tipY);
    path.arcTo(QRectF(cxF - r, cyF - r, r * 2, r * 2), 210.0f, 300.0f);
    path.lineTo(cxF, tipY);
    p.setBrush(QColor(211, 47, 47));
    p.setPen(QPen(QColor(183, 28, 28), 1.2));
    p.drawPath(path);
    float ir2 = r * 0.42f;
    p.setBrush(Qt::white); p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(cxF - ir2, cyF - ir2, ir2 * 2, ir2 * 2));
}

void FreezerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF body = bodyRect(), inner = innerRect();

    // Outer body
    QLinearGradient bg(body.topLeft(), body.bottomRight());
    bg.setColorAt(0, QColor(26, 74, 66)); bg.setColorAt(1, QColor(13, 46, 40));
    p.setBrush(bg); p.setPen(QPen(QColor(8, 30, 26), 2));
    p.drawRoundedRect(body, 14, 14);

    // Door handle
    QRectF handle(body.x()+10, body.y()+body.height()*0.25, 24, body.height()*0.50);
    QLinearGradient hg(handle.topLeft(), handle.topRight());
    hg.setColorAt(0, QColor(10,95,88,200)); hg.setColorAt(1, QColor(18,68,59,200));
    p.setBrush(hg); p.setPen(QPen(QColor(8,60,52),1));
    p.drawRoundedRect(handle, 8, 8);

    // Interior
    p.setBrush(QColor(240,248,245)); p.setPen(QPen(QColor(10,95,88,50),1));
    p.drawRoundedRect(inner, 6, 6);

    // Shelves
    for (int row = 0; row < N_SHELVES; ++row) {
        QRectF band = shelfBand(row);
        if (row > 0) {
            p.setPen(QPen(QColor(10,95,88,80), 1.5));
            p.drawLine(QPointF(band.x()+4, band.y()), QPointF(band.right()-4, band.y()));
        }
        // Label tab
        QRectF lr(band.x()+2, band.y()+8, 66, 22);
        QLinearGradient lg(lr.topLeft(), lr.bottomLeft());
        lg.setColorAt(0, QColor(10,95,88,220)); lg.setColorAt(1, QColor(18,68,59,220));
        p.setBrush(lg); p.setPen(Qt::NoPen);
        p.drawRoundedRect(lr, 6, 6);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(lr, Qt::AlignCenter, rowToLabel(row));

        // Slots
        for (int col = 0; col < N_SLOTS; ++col) {
            QRectF sr  = slotRectF(row, col);
            bool   sel = (row == m_selShelf && col == m_selSlot);
            bool   occ = (row < m_data.size() && col < m_data[row].size())
                         && m_data[row][col].occupied;
            QString danger = occ ? m_data[row][col].danger : QString();

            if (sel) {
                QLinearGradient sg(sr.topLeft(), sr.bottomRight());
                sg.setColorAt(0, QColor(63,140,248,230)); sg.setColorAt(1, QColor(30,80,200,230));
                p.setBrush(sg); p.setPen(QPen(Qt::white, 1.8));
            } else if (occ) {
                QColor c(61,158,147,200);
                if (danger == "BSL-3")       c = QColor(183,28,28,200);
                else if (danger == "BSL-2")  c = QColor(230,100,20,200);
                p.setBrush(c); p.setPen(QPen(c.darker(120), 0.8));
            } else {
                p.setBrush(QColor(200,230,225,60));
                p.setPen(QPen(QColor(10,95,88,60), 0.8, Qt::DashLine));
            }
            p.drawRoundedRect(sr, 4, 4);
        }
    }

    // Pin
    if (m_selShelf >= 0 && m_selSlot >= 0)
        drawPin(p, slotRectF(m_selShelf, m_selSlot));

    // Bottom strip
    QRectF strip(body.x()+4, body.bottom()-20, body.width()-8, 16);
    p.setBrush(QColor(10,95,88,160)); p.setPen(Qt::NoPen);
    p.drawRoundedRect(strip, 4, 4);
    if (m_selShelf >= 0 && m_selSlot >= 0) {
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(strip, Qt::AlignCenter,
            rowToLabel(m_selShelf) + "  |  Slot " + QString::number(m_selSlot+1));
    }
}

void FreezerWidget::mousePressEvent(QMouseEvent* e) {
    for (int r = 0; r < N_SHELVES; ++r)
        for (int c = 0; c < N_SLOTS; ++c)
            if (slotRectF(r,c).contains(e->position()))
                if (r < m_data.size() && c < m_data[r].size() && m_data[r][c].occupied)
                { emit slotClicked(r, c); return; }
}

// ═══════════════════════════════════════════════════════════════
// CongelateurDialog
// ═══════════════════════════════════════════════════════════════
static QWidget* card(QWidget* parent = nullptr) {
    auto* f = new QFrame(parent);
    f->setStyleSheet("QFrame{ background:rgba(255,255,255,0.82);"
                     "border:1px solid rgba(10,95,88,0.15); border-radius:14px; }");
    return f;
}
static QLabel* sectionTitle(const QString& t) {
    auto* l = new QLabel(t);
    l->setStyleSheet("font-weight:900; font-size:12px; color:rgba(10,95,88,0.90); padding:2px 0;");
    return l;
}
static void detailRow(QVBoxLayout* vl, const QString& lbl, QLabel*& out) {
    auto* row = new QWidget; auto* hl = new QHBoxLayout(row);
    hl->setContentsMargins(0,1,0,1); hl->setSpacing(6);
    auto* key = new QLabel(lbl + " :");
    key->setStyleSheet("color:rgba(10,95,88,0.70); font-size:11px; font-weight:700; min-width:72px;");
    out = new QLabel("—");
    out->setStyleSheet("color:rgba(0,0,0,0.75); font-size:11px; font-weight:900;");
    out->setWordWrap(true);
    hl->addWidget(key); hl->addWidget(out, 1);
    vl->addWidget(row);
}

CongelateurDialog::CongelateurDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("❄  AI Congélateur — Localisation des Échantillons");
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint);
    resize(1200, 720);
    setStyleSheet(
        "QDialog{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 #e4f2ef, stop:1 #cde8e3); }"
        "QScrollBar:vertical{ background:transparent; width:6px; }"
        "QScrollBar::handle:vertical{ background:rgba(10,95,88,0.30); border-radius:3px; }"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{ height:0; }");

    m_net  = new QNetworkAccessManager(this);
    m_crud = new CrudeBioSimple;
    connect(m_net, &QNetworkAccessManager::finished, this, &CongelateurDialog::onAiReply);

    auto* rootL = new QVBoxLayout(this);
    rootL->setContentsMargins(14,14,14,14); rootL->setSpacing(10);

    // Header
    auto* hdrCard = card(); auto* hdrL = new QHBoxLayout(hdrCard);
    hdrL->setContentsMargins(16,10,16,10);
    auto* hdrTitle = new QLabel("❄  AI Congélateur");
    hdrTitle->setStyleSheet("font-size:18px; font-weight:900; color:rgba(10,95,88,0.95);"
                            "border:none; background:transparent;");
    hdrL->addWidget(hdrTitle); hdrL->addStretch(1);
    m_aiInput = new QLineEdit;
    m_aiInput->setPlaceholderText("🔍  Localiser un échantillon (nom, ID, type)…");
    m_aiInput->setFixedWidth(320);
    m_aiInput->setStyleSheet("QLineEdit{ background:white; border:1.5px solid rgba(10,95,88,0.35);"
                             "border-radius:10px; padding:8px 14px; font-size:12px; }");
    m_aiBtn = new QPushButton("  Localiser par IA");
    m_aiBtn->setCursor(Qt::PointingHandCursor);
    m_aiBtn->setStyleSheet(
        "QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 rgba(10,95,88,0.92),stop:1 rgba(18,68,59,0.92));"
        "    color:white; font-weight:900; border:none; border-radius:10px; padding:9px 20px; }"
        "QPushButton:hover{ background:rgba(14,115,106,0.95); }"
        "QPushButton:disabled{ background:rgba(10,95,88,0.35); }");
    hdrL->addWidget(m_aiInput); hdrL->addWidget(m_aiBtn);
    rootL->addWidget(hdrCard);

    // Body
    auto* bodyL = new QHBoxLayout; bodyL->setSpacing(12);

    // Left
    auto* leftCard = card(); leftCard->setFixedWidth(240);
    auto* leftVL = new QVBoxLayout(leftCard);
    leftVL->setContentsMargins(10,12,10,12); leftVL->setSpacing(8);
    leftVL->addWidget(sectionTitle("Congélateurs"));
    m_freezerList = new QListWidget; m_freezerList->setMaximumHeight(160);
    m_freezerList->setStyleSheet(
        "QListWidget{ border:none; background:transparent; }"
        "QListWidget::item{ padding:8px 10px; border-radius:8px; font-weight:700;"
        "    color:rgba(0,0,0,0.65); font-size:12px; }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.95); }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }");
    leftVL->addWidget(m_freezerList);
    auto* sep1 = new QFrame; sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("border-top:1px solid rgba(10,95,88,0.12);");
    leftVL->addWidget(sep1);
    leftVL->addWidget(sectionTitle("Rechercher"));
    m_searchEdit = new QLineEdit; m_searchEdit->setPlaceholderText("Nom, ID ou Type…");
    m_searchEdit->setStyleSheet("QLineEdit{ background:rgba(255,255,255,0.80);"
                                "border:1px solid rgba(10,95,88,0.25); border-radius:8px;"
                                "padding:6px 10px; font-size:11px; }");
    leftVL->addWidget(m_searchEdit);
    m_searchFilter = new QComboBox;
    m_searchFilter->addItems({"Nom / ID", "Type", "Danger"});
    m_searchFilter->setStyleSheet("QComboBox{ background:rgba(255,255,255,0.80);"
                                  "border:1px solid rgba(10,95,88,0.25); border-radius:8px;"
                                  "padding:5px 10px; font-size:11px; }"
                                  "QComboBox::drop-down{ border:none; }");
    leftVL->addWidget(m_searchFilter);
    auto* sep2 = new QFrame; sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("border-top:1px solid rgba(10,95,88,0.12);");
    leftVL->addWidget(sep2);
    leftVL->addWidget(sectionTitle("Échantillons"));
    m_sampleList = new QListWidget;
    m_sampleList->setStyleSheet(
        "QListWidget{ border:none; background:transparent; }"
        "QListWidget::item{ padding:6px 8px; border-radius:7px; font-size:10px; color:rgba(0,0,0,0.65); }"
        "QListWidget::item:selected{ background:rgba(10,95,88,0.18); color:rgba(10,95,88,0.90); font-weight:700; }"
        "QListWidget::item:hover{ background:rgba(10,95,88,0.08); }");
    leftVL->addWidget(m_sampleList, 1);
    bodyL->addWidget(leftCard);

    // Center
    auto* centerCard = card(); auto* centerVL = new QVBoxLayout(centerCard);
    centerVL->setContentsMargins(14,14,14,10); centerVL->setSpacing(8);
    auto* centerTitle = sectionTitle("Localisation de l'Échantillon");
    centerTitle->setStyleSheet("font-weight:900; font-size:13px; color:rgba(10,95,88,0.90);"
                               "border:none; background:transparent;");
    centerVL->addWidget(centerTitle);
    m_freezerWidget = new FreezerWidget;
    centerVL->addWidget(m_freezerWidget, 1);
    m_statusBar = new QLabel("Sélectionnez un congélateur, puis cliquez sur un échantillon.");
    m_statusBar->setAlignment(Qt::AlignCenter); m_statusBar->setFixedHeight(34);
    m_statusBar->setStyleSheet(
        "background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "    stop:0 rgba(10,95,88,0.85),stop:1 rgba(18,68,59,0.85));"
        "color:white; font-weight:900; font-size:12px; border-radius:8px; padding:0 14px;");
    centerVL->addWidget(m_statusBar);
    bodyL->addWidget(centerCard, 1);

    // Right
    auto* rightCard = card(); rightCard->setFixedWidth(260);
    auto* rightVL = new QVBoxLayout(rightCard);
    rightVL->setContentsMargins(14,12,14,12); rightVL->setSpacing(6);
    m_pinIcon = new QLabel("📍"); m_pinIcon->setAlignment(Qt::AlignCenter);
    m_pinIcon->setStyleSheet("font-size:28px; border:none; background:transparent;");
    m_pinIcon->hide(); rightVL->addWidget(m_pinIcon);
    rightVL->addWidget(sectionTitle("Détails de l'Échantillon"));
    detailRow(rightVL, "ID",         m_detId);
    detailRow(rightVL, "Type",       m_detType);
    detailRow(rightVL, "Organisme",  m_detOrg);
    detailRow(rightVL, "Étage",      m_detEtage);
    detailRow(rightVL, "Slot",       m_detSlot);
    detailRow(rightVL, "Temp.",      m_detTemp);
    detailRow(rightVL, "Danger",     m_detDanger);
    auto* sep3 = new QFrame; sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet("border-top:1px solid rgba(10,95,88,0.15); margin:4px 0; background:transparent;");
    rightVL->addWidget(sep3);
    rightVL->addWidget(sectionTitle("Informations supplémentaires"));
    detailRow(rightVL, "Stockage",   m_detDateCol);
    detailRow(rightVL, "Expiration", m_detDateExp);
    auto* sep4 = new QFrame; sep4->setFrameShape(QFrame::HLine);
    sep4->setStyleSheet("border-top:1px solid rgba(10,95,88,0.15); margin:4px 0; background:transparent;");
    rightVL->addWidget(sep4);
    rightVL->addWidget(sectionTitle("Réponse IA"));
    m_aiResp = new QLabel("Posez une question ci-dessus\npour localiser un échantillon.");
    m_aiResp->setWordWrap(true); m_aiResp->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    m_aiResp->setStyleSheet("color:rgba(0,0,0,0.65); font-size:11px; padding:8px;"
                            "background:rgba(10,95,88,0.07); border-radius:8px;");
    rightVL->addWidget(m_aiResp, 1);
    bodyL->addWidget(rightCard);
    rootL->addLayout(bodyL, 1);

    connect(m_freezerList,  &QListWidget::itemClicked,  this, &CongelateurDialog::onFreezerClicked);
    connect(m_freezerWidget,&FreezerWidget::slotClicked,this, &CongelateurDialog::onSlotClicked);
    connect(m_sampleList,   &QListWidget::itemClicked,  this, &CongelateurDialog::onSampleListClicked);
    connect(m_searchEdit,   &QLineEdit::textChanged,    this, &CongelateurDialog::onSearchChanged);
    connect(m_aiBtn,        &QPushButton::clicked,      this, &CongelateurDialog::onAiSearch);
    connect(m_aiInput,      &QLineEdit::returnPressed,  this, &CongelateurDialog::onAiSearch);

    loadFreezers();
}

void CongelateurDialog::refresh() {
    QString prev = m_currentCong; loadFreezers();
    if (!prev.isEmpty()) loadFreezer(prev);
}

void CongelateurDialog::loadFreezers() {
    m_freezerList->clear();
    QSqlQuery q;
    q.prepare("SELECT DISTINCT EMPLACEMENT_STOCKAGE FROM ECHANTILLONS "
              "WHERE EMPLACEMENT_STOCKAGE LIKE 'Cong:%'");
    if (!q.exec()) return;
    QStringList seen;
    while (q.next()) {
        QString c = parseCong(q.value(0).toString());
        if (!c.isEmpty() && !seen.contains(c)) {
            seen.append(c);
            auto* item = new QListWidgetItem("❄  " + c);
            item->setData(Qt::UserRole, c);
            m_freezerList->addItem(item);
        }
    }
    if (m_freezerList->count() == 0) {
        auto* item = new QListWidgetItem("Aucun congélateur");
        item->setFlags(Qt::NoItemFlags); m_freezerList->addItem(item);
    } else {
        m_freezerList->setCurrentRow(0);
        loadFreezer(m_freezerList->item(0)->data(Qt::UserRole).toString());
    }
}

void CongelateurDialog::loadFreezer(const QString& cong)
{
    m_currentCong = cong;
    m_slotData.clear();
    m_slotData.resize(FreezerWidget::N_SHELVES, QVector<SlotInfo>(FreezerWidget::N_SLOTS));
    QVector<int> nextSlot(FreezerWidget::N_SHELVES, 0);

    QSqlQuery q;
    q.prepare(
        "SELECT REFERENCE AS REFERENCE_ECHANTILLON, TYPE_ECHANTILLON, SOURCE AS ORGANISME_SOURCE,"
        "       EMPLACEMENT_STOCKAGE AS EMPLACEMENT_DE_STOCKAGE, TEMPERATURE_STOCKAGE AS TEMPERATURE_DE_STOCKAGE,"
        "       QUANTITE_RESTANTE, NIVEAU_DANGEROSITE AS NIVEAU_DE_DANGEROSITE,"
        "       DATE_COLLECTE AS DATE_DE_COLLECTE, DATE_EXPIRATION "
        "FROM ECHANTILLONS WHERE EMPLACEMENT_STOCKAGE LIKE :pat");
    q.bindValue(":pat", QString("Cong:%1/%").arg(cong));

    QStringList ctxParts;
    if (q.exec()) {
        while (q.next()) {
            QString emp = q.value(3).toString();
            if (parseCong(emp) != cong) continue;
            int row = etageToRow(parseEtag(emp));
            int col = nextSlot[row];
            if (col >= FreezerWidget::N_SLOTS) continue;
            nextSlot[row]++;
            SlotInfo si;
            si.reference      = q.value(0).toString();
            si.type           = q.value(1).toString();
            si.organisme      = q.value(2).toString();
            si.emplacement    = emp;
            si.temperature    = q.value(4).toString();
            si.quantite       = q.value(5).toInt();
            si.danger         = q.value(6).toString();
            si.dateCollecte   = q.value(7).toDate().toString("dd/MM/yyyy");
            si.dateExpiration = q.value(8).toDate().toString("dd/MM/yyyy");
            si.etage          = parseEtag(emp);
            m_slotData[row][col] = si;
            ctxParts << QString("Réf:%1 Type:%2 Étage:%3 Slot:%4")
                        .arg(si.reference, si.type, si.etage).arg(col+1);
        }
    }
    m_lastContext = ctxParts.join(" | ");

    QVector<QVector<FreezerWidget::Slot>> fwData(FreezerWidget::N_SHELVES,
        QVector<FreezerWidget::Slot>(FreezerWidget::N_SLOTS));
    for (int r = 0; r < FreezerWidget::N_SHELVES; ++r)
        for (int c = 0; c < FreezerWidget::N_SLOTS; ++c) {
            fwData[r][c].occupied  = !m_slotData[r][c].reference.isEmpty();
            fwData[r][c].reference = m_slotData[r][c].reference;
            fwData[r][c].danger    = m_slotData[r][c].danger;
        }
    m_freezerWidget->setData(fwData);
    clearDetails();
    m_statusBar->setText(QString("Congélateur  %1  —  %2 échantillon(s)")
                         .arg(cong).arg(ctxParts.size()));
    rebuildSampleList();
}

void CongelateurDialog::rebuildSampleList(const QString& filter) {
    m_sampleList->clear();
    QString ft = m_searchFilter ? m_searchFilter->currentText() : "Nom / ID";
    for (int r = 0; r < FreezerWidget::N_SHELVES; ++r)
        for (int c = 0; c < FreezerWidget::N_SLOTS; ++c) {
            const SlotInfo& si = m_slotData[r][c];
            if (si.reference.isEmpty()) continue;
            if (!filter.isEmpty()) {
                QString hay = (ft=="Type") ? si.type : (ft=="Danger") ? si.danger : si.reference;
                if (!hay.contains(filter, Qt::CaseInsensitive)) continue;
            }
            auto* item = new QListWidgetItem(si.reference + "\n" + si.type + "  |  " + rowToLabel(r));
            item->setData(Qt::UserRole, QPoint(r, c));
            m_sampleList->addItem(item);
        }
}

void CongelateurDialog::showDetails(int shelf, int slot) {
    const SlotInfo& si = m_slotData[shelf][slot];
    if (si.reference.isEmpty()) { clearDetails(); return; }
    m_pinIcon->show();
    m_detId      ->setText(si.reference);
    m_detType    ->setText(si.type.isEmpty()        ? "—" : si.type);
    m_detOrg     ->setText(si.organisme.isEmpty()   ? "—" : si.organisme);
    m_detEtage   ->setText(rowToLabel(shelf));
    m_detSlot    ->setText(QString("Slot %1  /  Rack %2").arg(slot+1).arg(slot/4+1));
    m_detTemp    ->setText(si.temperature.isEmpty() ? "—" : si.temperature + " °C");
    m_detDanger  ->setText(si.danger.isEmpty()      ? "—" : si.danger);
    m_detDateCol ->setText(si.dateCollecte.isEmpty()   ? "—" : si.dateCollecte);
    m_detDateExp ->setText(si.dateExpiration.isEmpty() ? "—" : si.dateExpiration);
    m_statusBar->setText(rowToLabel(shelf) + "  |  Slot " + QString::number(slot+1) + "  |  " + si.reference);
    for (int i = 0; i < m_sampleList->count(); ++i) {
        QPoint p = m_sampleList->item(i)->data(Qt::UserRole).toPoint();
        if (p.x()==shelf && p.y()==slot) { m_sampleList->setCurrentRow(i); break; }
    }
}

void CongelateurDialog::clearDetails() {
    if (m_pinIcon)    m_pinIcon->hide();
    if (m_detId)      m_detId      ->setText("—");
    if (m_detType)    m_detType    ->setText("—");
    if (m_detOrg)     m_detOrg     ->setText("—");
    if (m_detEtage)   m_detEtage   ->setText("—");
    if (m_detSlot)    m_detSlot    ->setText("—");
    if (m_detTemp)    m_detTemp    ->setText("—");
    if (m_detDanger)  m_detDanger  ->setText("—");
    if (m_detDateCol) m_detDateCol ->setText("—");
    if (m_detDateExp) m_detDateExp ->setText("—");
    if (m_aiResp)     m_aiResp->setText("Posez une question ci-dessus\npour localiser un échantillon.");
}

void CongelateurDialog::onFreezerClicked(QListWidgetItem* item) {
    loadFreezer(item->data(Qt::UserRole).toString());
}
void CongelateurDialog::onSlotClicked(int shelf, int slot) {
    m_freezerWidget->selectSlot(shelf, slot); showDetails(shelf, slot);
}
void CongelateurDialog::onSampleListClicked(QListWidgetItem* item) {
    QPoint p = item->data(Qt::UserRole).toPoint(); onSlotClicked(p.x(), p.y());
}
void CongelateurDialog::onSearchChanged(const QString& text) {
    rebuildSampleList(text.trimmed());
}
void CongelateurDialog::onAiSearch() {
    QString q = m_aiInput->text().trimmed(); if (q.isEmpty()) return;
    m_aiResp->setText("⏳ Recherche en cours…"); m_aiBtn->setEnabled(false);
    callGroq(q, m_lastContext);
}
void CongelateurDialog::callGroq(const QString& userMsg, const QString& context) {
    QString system = QString(
        "Tu es un assistant de laboratoire. Congélateur : %1. Échantillons : %2. "
        "Aide à localiser. Réponds en français, 2-3 phrases. Précise étage et slot."
    ).arg(m_currentCong.isEmpty()?"non sélectionné":m_currentCong,
          context.isEmpty()?"aucun":context);
    QJsonArray msgs = {
        QJsonObject{{"role","system"},{"content",system}},
        QJsonObject{{"role","user"},  {"content",userMsg}}
    };
    QJsonObject body{{"model",GROQ_MODEL},{"messages",msgs},{"max_tokens",300},{"temperature",0.3}};
    QUrl reqUrl(GROQ_URL); QNetworkRequest req(reqUrl);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    req.setRawHeader("Authorization",("Bearer "+GROQ_KEY).toUtf8());
    QNetworkReply* rpl = m_net->post(req, QJsonDocument(body).toJson());
    connect(rpl,&QNetworkReply::sslErrors,rpl,[rpl](const QList<QSslError>&){rpl->ignoreSslErrors();});
}
void CongelateurDialog::onAiReply(QNetworkReply* reply) {
    m_aiBtn->setEnabled(true);
    QByteArray data = reply->readAll(); bool netErr=(reply->error()!=QNetworkReply::NoError);
    QString errStr=reply->errorString(); reply->deleteLater();
    if (netErr){m_aiResp->setText("Erreur réseau : "+errStr);return;}
    QJsonObject root=QJsonDocument::fromJson(data).object();
    if (root.contains("error")){m_aiResp->setText("Erreur API : "+root["error"].toObject()["message"].toString());return;}
    QString answer=root["choices"].toArray().first().toObject()["message"].toObject()["content"].toString().trimmed();
    m_aiResp->setText(answer);
    for (int r=0;r<m_slotData.size();++r)
        for (int c=0;c<m_slotData[r].size();++c)
            if (!m_slotData[r][c].reference.isEmpty()&&answer.contains(m_slotData[r][c].reference,Qt::CaseInsensitive))
            {onSlotClicked(r,c);return;}
}
