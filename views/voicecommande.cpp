#include "voicecommande.h"
#include "apiconfig.h"

#include <QPainter>
#include <QPainterPath>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSslConfiguration>
#include <QSslSocket>
#include <QSslError>
#include <QUrl>
#include <QApplication>
#include <QScreen>

// ─── WAV helpers ──────────────────────────────────────────────────
QByteArray VoiceCommand::le32(quint32 v)
{
    QByteArray b(4, '\0');
    b[0] = char(v & 0xFF); b[1] = char((v >> 8) & 0xFF);
    b[2] = char((v >> 16) & 0xFF); b[3] = char((v >> 24) & 0xFF);
    return b;
}
QByteArray VoiceCommand::le16(quint16 v)
{
    QByteArray b(2, '\0');
    b[0] = char(v & 0xFF); b[1] = char((v >> 8) & 0xFF);
    return b;
}
QByteArray VoiceCommand::buildWav(const QByteArray& pcm, int sr, int ch) const
{
    const quint32 dataSize   = quint32(pcm.size());
    const quint16 bps        = 16;
    const quint32 byteRate   = quint32(sr * ch * (bps / 8));
    const quint16 blockAlign = quint16(ch * (bps / 8));
    QByteArray wav;
    wav.reserve(int(44 + dataSize));
    wav.append("RIFF", 4); wav.append(le32(36 + dataSize));
    wav.append("WAVE", 4); wav.append("fmt ", 4); wav.append(le32(16));
    wav.append(le16(1));   wav.append(le16(quint16(ch)));
    wav.append(le32(quint32(sr))); wav.append(le32(byteRate));
    wav.append(le16(blockAlign));  wav.append(le16(bps));
    wav.append("data", 4); wav.append(le32(dataSize)); wav.append(pcm);
    return wav;
}

// ─── Local C++ command parser (zero network, zero latency) ────────

// Normalise French text: lowercase + explicit accent stripping + punctuation removal.
// Uses a direct character-by-character replacement table — robust regardless of
// source encoding or platform Unicode normalization support.
static QString norm(const QString& s)
{
    // Map of accented/special chars -> ASCII equivalent (code point pairs)
    // Each entry: {from, to}
    static const struct { ushort from; char to; } kMap[] = {
        // lowercase accented (already lowercase after toLower)
        {0x00E0,'a'},{0x00E1,'a'},{0x00E2,'a'},{0x00E3,'a'},{0x00E4,'a'},{0x00E5,'a'},
        {0x00E8,'e'},{0x00E9,'e'},{0x00EA,'e'},{0x00EB,'e'},
        {0x00EC,'i'},{0x00ED,'i'},{0x00EE,'i'},{0x00EF,'i'},
        {0x00F2,'o'},{0x00F3,'o'},{0x00F4,'o'},{0x00F5,'o'},{0x00F6,'o'},
        {0x00F9,'u'},{0x00FA,'u'},{0x00FB,'u'},{0x00FC,'u'},
        {0x00FD,'y'},{0x00FF,'y'},
        {0x00E7,'c'},                        // c cedilla
        {0x00F1,'n'},                        // n tilde
        {0x0153,'o'},{0x00E6,'a'},           // oe/ae ligatures (simplified)
        // uppercase accented (toLower will turn these into lowercase first, but safety)
        {0x00C0,'a'},{0x00C1,'a'},{0x00C2,'a'},{0x00C3,'a'},{0x00C4,'a'},{0x00C5,'a'},
        {0x00C8,'e'},{0x00C9,'e'},{0x00CA,'e'},{0x00CB,'e'},
        {0x00CC,'i'},{0x00CD,'i'},{0x00CE,'i'},{0x00CF,'i'},
        {0x00D2,'o'},{0x00D3,'o'},{0x00D4,'o'},{0x00D5,'o'},{0x00D6,'o'},
        {0x00D9,'u'},{0x00DA,'u'},{0x00DB,'u'},{0x00DC,'u'},
        {0x00C7,'c'},                        // C cedilla
        {0x00D1,'n'},                        // N tilde
        {0x0152,'o'},{0x00C6,'a'},           // OE/AE ligatures
    };

    QString out;
    out.reserve(s.size());
    for (const QChar& qc : s) {
        const ushort u = qc.unicode();

        // Skip punctuation and symbols (keep only letters, digits, space)
        if (u == '.' || u == ',' || u == '!' || u == '?' || u == '\'' ||
            u == 0x2019 || u == 0x2018 || u == '-' || u == ':' || u == ';')
            continue;

        // Try accent replacement
        bool replaced = false;
        for (const auto& m : kMap) {
            if (u == m.from) {
                out.append(QLatin1Char(m.to));
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            // Lowercase and append as-is
            out.append(qc.toLower());
        }
    }

    return out;
}

// Check if normalised text contains any of the keywords
static bool has(const QString& t, std::initializer_list<const char*> kws)
{
    for (const char* k : kws)
        if (t.contains(QLatin1String(k))) return true;
    return false;
}

struct Cmd { QString action, module, desc, searchText; };

static bool voiceModuleAllowedForRole(const QString& role, const QString& module)
{
    if (module.isEmpty()) return true;
    if (module == "congelateur") return true;

    if (role == "Responsable") return true;
    if (module == "employee")    return role == "RH";
    if (module == "publication") return role == "Chercheur";
    if (module == "experience")  return role == "Chercheur";
    if (module == "projet")      return role == "Chercheur";
    if (module == "biosample")   return role == "Technicien";
    if (module == "equipement")  return role == "Technicien";

    return false;
}

static Cmd parseCmd(const QString& raw, const QString& ctx)
{
    const QString t = norm(raw);

    // ── 1. Global actions (no module) ─────────────────────────────
    if (has(t, {"congelateur","frigo","freezer","cong ia","ai cong"}))
        return {"congelateur", "", "Ouvrir Congélateur IA", ""};

    if (has(t, {"chatbot","chat bot","assistant ia","aide ia","bot ia","ouvre chat","parler ia"}))
        return {"chatbot", "", "Ouvrir Chatbot IA", ""};

    if (has(t, {"deconnecter","deconnexion","logout","sign out","deloger"}))
        return {"logout", "", "Déconnexion", ""};

    // save / cancel — only if no add/edit keyword present
    if (has(t, {"enregistre","sauvegarder","valider","valide","sauver","confirmer","terminer"})
        && !has(t, {"ajoute","nouveau","creer","modifie","edite"}))
        return {"save", "", "Enregistrer", ""};

    if (has(t, {"annule","annuler","abandonner","stopper"})
        && !has(t, {"ajoute","nouveau","creer"}))
        return {"cancel", "", "Annuler", ""};

    // ── 2. Detect module ──────────────────────────────────────────
    QString mod;
    if      (has(t, {"biosample","bio sample","echantillon","prelevement","tube","biologie","bio"}))
        mod = "biosample";
    else if (has(t, {"employe","personnel","collaborateur","staff","agent","membre","employes"}))
        mod = "employee";
    else if (has(t, {"equipement","materiel","instrument","appareil","dispositif","outil","machine"}))
        mod = "equipement";
    else if (has(t, {"experience","manip","manipulation","protocole","essai","experiences"}))
        mod = "experience";
    else if (has(t, {"projet","programme","initiative","mission","dossier","projets"}))
        mod = "projet";
    else if (has(t, {"publication","article","rapport","these","papier","recherche","publications"}))
        mod = "publication";

    // Fallback to current page context if no module detected
    if (mod.isEmpty()) mod = ctx;

    // ── 3. Detect action ──────────────────────────────────────────
    QString act;
    if      (has(t, {"ajoute","ajouter","nouveau","nouvelle","creer","inserer","saisir","new","add"}))
        act = "add";
    else if (has(t, {"modifie","modifier","edite","editer","change","update","corriger","rectifier","mettre a jour"}))
        act = "edit";
    else if (has(t, {"supprime","supprimer","efface","effacer","enleve","enlever","retirer","virer","eliminer","delete"}))
        act = "delete";
    else if (has(t, {"details","infos","informations","fiche","consulter","voir plus","descriptif"}))
        act = "details";
    else if (has(t, {"stats","statistiques","graphique","graphe","dashboard","tableau de bord","analyse","bilan"}))
        act = "stats";
    else if (has(t, {"pdf","exporte","imprimer","telecharger","generer"}))
        act = "export_pdf";
    else if (has(t, {"recherche","cherche","filtre","trouver","trouve","chercher","filtrer","scanner"}))
        act = "search";
    else if (has(t, {"actualise","rafraichir","recharger","reload","actualiser","recharge"}))
        act = "refresh";
    else
        act = "navigate";

    // ── 4. Extract search text ────────────────────────────────────
    QString srch;
    if (act == "search") {
        const QStringList skws = {"recherche","cherche","filtre","trouver","chercher","filtrer"};
        for (const QString& sk : skws) {
            int idx = t.indexOf(sk);
            if (idx >= 0) {
                srch = raw.mid(idx + sk.length()).trimmed();
                // Remove trailing module keyword if it ends with one
                for (const char* mk : {"biosample","employee","equipement","experience","projet","publication"})
                    if (norm(srch).endsWith(mk)) srch.chop(QString::fromLatin1(mk).length());
                srch = srch.trimmed();
                break;
            }
        }
    }

    // ── 5. Description ───────────────────────────────────────────
    using P = QPair<QString,QString>;
    static const QMap<P,QString> descs = {
        {{"navigate","biosample"},   "Ouvrir BioSample"},
        {{"navigate","employee"},    "Ouvrir Employés"},
        {{"navigate","equipement"},  "Ouvrir Équipements"},
        {{"navigate","experience"},  "Ouvrir Expériences"},
        {{"navigate","projet"},      "Ouvrir Projets"},
        {{"navigate","publication"}, "Ouvrir Publications"},
        {{"add","biosample"},   "Ajouter un échantillon"},
        {{"add","employee"},    "Ajouter un employé"},
        {{"add","equipement"},  "Ajouter un équipement"},
        {{"add","experience"},  "Ajouter une expérience"},
        {{"add","projet"},      "Ajouter un projet"},
        {{"add","publication"}, "Ajouter une publication"},
        {{"edit","biosample"},   "Modifier BioSample"},
        {{"edit","employee"},    "Modifier un employé"},
        {{"edit","equipement"},  "Modifier un équipement"},
        {{"edit","experience"},  "Modifier une expérience"},
        {{"edit","projet"},      "Modifier un projet"},
        {{"edit","publication"}, "Modifier une publication"},
        {{"delete","biosample"},   "Supprimer BioSample"},
        {{"delete","employee"},    "Supprimer un employé"},
        {{"delete","equipement"},  "Supprimer un équipement"},
        {{"delete","experience"},  "Supprimer une expérience"},
        {{"delete","projet"},      "Supprimer un projet"},
        {{"delete","publication"}, "Supprimer une publication"},
        {{"details","experience"},  "Détails expérience"},
        {{"details","equipement"},  "Détails équipement"},
        {{"details","projet"},      "Détails projet"},
        {{"details","publication"}, "Détails publication"},
        {{"stats","biosample"},   "Statistiques BioSample"},
        {{"stats","employee"},    "Statistiques Employés"},
        {{"stats","experience"},  "Statistiques Expériences"},
        {{"stats","publication"}, "Statistiques Publications"},
        {{"export_pdf","biosample"}, "Exporter PDF BioSample"},
        {{"refresh",""},  "Actualiser"},
        {{"search",""},   "Rechercher"},
    };
    QString d = descs.value({act, mod});
    if (d.isEmpty()) {
        QString al = act; al[0] = al[0].toUpper();
        d = al.replace('_',' ') + (mod.isEmpty() ? "" : " " + mod);
    }

    return {act, mod, d, srch};
}

// ─── Constructor ──────────────────────────────────────────────────
VoiceCommand::VoiceCommand(QWidget* parent)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    Q_UNUSED(parent)
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(270, 310);

    m_net = new QNetworkAccessManager(this);
    connect(m_net, &QNetworkAccessManager::finished, this, &VoiceCommand::onNetworkReply);

    m_tts = new QTextToSpeech(this);
    m_tts->setLocale(QLocale(QLocale::French));

    // ── Root layout ──
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(0);

    m_panel = new QWidget;
    QVBoxLayout* panelL = new QVBoxLayout(m_panel);
    panelL->setContentsMargins(14, 10, 14, 12);
    panelL->setSpacing(7);

    // ── Drag handle / header ──
    m_dragHandle = new QWidget;
    m_dragHandle->setFixedHeight(32);
    m_dragHandle->setCursor(Qt::SizeAllCursor);
    m_dragHandle->setStyleSheet("background:transparent;");
    QHBoxLayout* headerL = new QHBoxLayout(m_dragHandle);
    headerL->setContentsMargins(0,0,0,0);
    headerL->setSpacing(6);

    QLabel* ico = new QLabel("🎙");
    ico->setStyleSheet("font-size:16px; background:transparent; color:white;");
    ico->setAttribute(Qt::WA_TransparentForMouseEvents);

    QLabel* title = new QLabel("Commandes Vocales");
    title->setStyleSheet("color:white; font-weight:900; font-size:12px; background:transparent;");
    title->setAttribute(Qt::WA_TransparentForMouseEvents);

    // Drag hint dots
    QLabel* dots = new QLabel("⋮⋮");
    dots->setStyleSheet("color:rgba(255,255,255,0.30); font-size:14px; background:transparent;");
    dots->setAttribute(Qt::WA_TransparentForMouseEvents);

    QPushButton* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(20, 20);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        "QPushButton{ background:rgba(220,55,48,0.85); color:white; border:none;"
        " border-radius:10px; font-size:10px; font-weight:900; }"
        "QPushButton:hover{ background:rgba(255,75,65,1); }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::hide);

    headerL->addWidget(ico);
    headerL->addWidget(title, 1);
    headerL->addWidget(dots);
    headerL->addWidget(closeBtn);
    panelL->addWidget(m_dragHandle);

    // ── Separator ──
    QFrame* sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("border:1px solid rgba(255,255,255,0.12); background:transparent;");
    panelL->addWidget(sep);

    // ── Big microphone button ──
    m_micBtn = new QPushButton("🎤");
    m_micBtn->setFixedSize(68, 68);
    m_micBtn->setCursor(Qt::PointingHandCursor);
    m_micBtn->setCheckable(true);
    m_micBtn->setStyleSheet(
        "QPushButton{"
        "  background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 rgba(99,102,241,0.90), stop:1 rgba(168,85,247,0.90));"
        "  border:none; border-radius:34px; font-size:26px;"
        "}"
        "QPushButton:hover{"
        "  background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 rgba(129,132,255,1), stop:1 rgba(192,115,255,1));"
        "}"
        "QPushButton:checked{"
        "  background:qlineargradient(x1:0,y1:0,x2:1,y2:1,"
        "    stop:0 rgba(239,68,68,0.95), stop:1 rgba(220,38,38,0.95));"
        "}");
    connect(m_micBtn, &QPushButton::clicked, this, &VoiceCommand::toggleRecording);

    QHBoxLayout* micRow = new QHBoxLayout;
    micRow->addStretch(1);
    micRow->addWidget(m_micBtn);
    micRow->addStretch(1);
    panelL->addLayout(micRow);

    // ── Status ──
    m_statusLbl = new QLabel("Appuyez pour parler");
    m_statusLbl->setAlignment(Qt::AlignCenter);
    m_statusLbl->setWordWrap(true);
    m_statusLbl->setStyleSheet(
        "color:rgba(200,210,255,0.85); font-size:11px; font-weight:700;"
        " background:transparent; padding:2px;");
    panelL->addWidget(m_statusLbl);

    // ── Transcript ──
    m_transcriptLbl = new QLabel;
    m_transcriptLbl->setAlignment(Qt::AlignCenter);
    m_transcriptLbl->setWordWrap(true);
    m_transcriptLbl->setStyleSheet(
        "color:rgba(255,255,255,0.70); font-size:10px; font-style:italic;"
        " background:rgba(255,255,255,0.07); border-radius:6px; padding:4px 6px;");
    m_transcriptLbl->hide();
    panelL->addWidget(m_transcriptLbl);

    // ── Action result ──
    m_actionLbl = new QLabel;
    m_actionLbl->setAlignment(Qt::AlignCenter);
    m_actionLbl->setWordWrap(true);
    m_actionLbl->setStyleSheet(
        "color:#4ade80; font-size:11px; font-weight:800;"
        " background:rgba(74,222,128,0.10); border-radius:6px; padding:4px 8px;");
    m_actionLbl->hide();
    panelL->addWidget(m_actionLbl);

    // ── Hint ──
    QLabel* hint = new QLabel(
        "Ex: \"Ouvre BioSample\" · \"Ajoute un employé\"\n"
        "\"Modifie l'expérience\" · \"Supprime l'équipement\"\n"
        "\"Stats publications\" · \"Exporte PDF\" · \"Chatbot\"\n"
        "\"Ouvre congélateur\" · \"Enregistre\" · \"Déconnecte\"");
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet(
        "color:rgba(150,160,200,0.55); font-size:9px;"
        " background:transparent; padding:2px;");
    panelL->addWidget(hint);

    root->addWidget(m_panel);

    // ── Pulse timer ──
    m_pulseTimer = new QTimer(this);
    m_pulseTimer->setInterval(25);
    connect(m_pulseTimer, &QTimer::timeout, this, [this](){
        m_pulseAlpha += m_pulseUp ? 7 : -7;
        if (m_pulseAlpha >= 180) m_pulseUp = false;
        if (m_pulseAlpha <= 30)  m_pulseUp = true;
        update();
    });

    // ── Position: bottom-right of primary screen ──
    positionBottomRight();
}

VoiceCommand::~VoiceCommand()
{
    // Stop pulse animation immediately
    m_pulseTimer->stop();

    // Disconnect network callbacks before any teardown so no slot fires
    // on a partially-destroyed object while Qt unwinds child objects.
    if (m_net)
        disconnect(m_net, &QNetworkAccessManager::finished,
                   this,  &VoiceCommand::onNetworkReply);

    // Abort any in-flight STT request and delete the reply directly
    // (it has no Qt parent, so deleteLater is required — safe here because
    // we have already disconnected the finished signal above).
    if (m_pendingReply) {
        m_pendingReply->abort();
        m_pendingReply->deleteLater();
        m_pendingReply = nullptr;
    }

    // Stop TTS
    if (m_tts && m_tts->state() == QTextToSpeech::Speaking)
        m_tts->stop();

    // Release audio device synchronously — delete, NOT deleteLater,
    // so the OS device handle is freed before the next app launch tries
    // to open it again.
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    if (m_audioBuffer) {
        m_audioBuffer->close();
        delete m_audioBuffer;
        m_audioBuffer = nullptr;
    }
}

void VoiceCommand::setCurrentContext(int pageIdx, const QString& pageName)
{
    m_contextPage = pageIdx;
    m_contextName = pageName;
}

void VoiceCommand::setCurrentRole(const QString& role)
{
    m_currentRole = role.trimmed();
}

void VoiceCommand::positionBottomRight()
{
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    move(screen.right() - width() - 16, screen.bottom() - height() - 56);
}

// ─── Paint ────────────────────────────────────────────────────────
void VoiceCommand::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect card = rect().adjusted(8, 8, -8, -8);
    QPainterPath path;
    path.addRoundedRect(card, 16, 16);
    p.fillPath(path, QColor(12, 10, 36, 235));

    // Border glow
    p.setPen(QPen(QColor(99, 102, 241, 90), 1.5));
    p.drawPath(path);

    // Pulse ring while recording
    if (m_isRecording) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(239, 68, 68, m_pulseAlpha));
        QPoint c = rect().center();
        c.setY(c.y() - 14);
        int r = 50 + (180 - m_pulseAlpha) / 7;
        p.drawEllipse(c, r, r);
    }
}

// ─── Drag ─────────────────────────────────────────────────────────
void VoiceCommand::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    }
}
void VoiceCommand::mouseMoveEvent(QMouseEvent* e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton)) {
        move(e->globalPosition().toPoint() - m_dragPos);
        e->accept();
    }
}
void VoiceCommand::mouseReleaseEvent(QMouseEvent*) { m_dragging = false; }

// ─── Show / Hide events ───────────────────────────────────────────
// Called when the widget is hidden (logout, window close, etc.).
// Ensures the microphone and any pending network request are fully released
// so they are available clean at the next show().
void VoiceCommand::hideEvent(QHideEvent* e)
{
    // If the user hid the panel while recording, stop cleanly
    if (m_isRecording) {
        m_isRecording = false;
        m_pulseTimer->stop();
        m_micBtn->setChecked(false);
        m_micBtn->setText("🎤");
        if (m_audioSource) {
            m_audioSource->stop();
            delete m_audioSource;
            m_audioSource = nullptr;
        }
        if (m_audioBuffer) {
            m_audioBuffer->close();
            delete m_audioBuffer;
            m_audioBuffer = nullptr;
        }
    }

    // Abort any in-flight STT request — result is now irrelevant
    if (m_pendingReply) {
        QNetworkReply* old = m_pendingReply;
        m_pendingReply = nullptr;
        old->abort();
    }

    // Stop TTS
    if (m_tts && m_tts->state() == QTextToSpeech::Speaking)
        m_tts->stop();

    QWidget::hideEvent(e);
}

// Called when the widget becomes visible again (user re-opens the panel).
// Resets UI to idle state so it always looks clean regardless of what
// happened before the last hide().
void VoiceCommand::showEvent(QShowEvent* e)
{
    // Reset UI to idle — no stale "Transcription…" or action labels
    m_micBtn->setChecked(false);
    m_micBtn->setText("🎤");
    m_transcriptLbl->hide();
    m_actionLbl->hide();
    setStatus("Appuyez pour parler");
    update();
    QWidget::showEvent(e);
}

// ─── Recording ────────────────────────────────────────────────────
void VoiceCommand::toggleRecording()
{
    if (m_isRecording) stopRecordingAndProcess();
    else               startRecording();
}

void VoiceCommand::startRecording()
{
    if (m_isRecording) return;

    // ── 1. Abort any in-flight STT request so its reply won't be processed ──
    if (m_pendingReply) {
        QNetworkReply* old = m_pendingReply;
        m_pendingReply = nullptr;   // clear first — onNetworkReply may fire sync
        old->abort();               // emits finished(OperationCanceledError)
        // reply cleaned up in onNetworkReply via deleteLater
    }

    // ── 2. Stop TTS so it does not bleed into the microphone ──────────────
    if (m_tts && m_tts->state() == QTextToSpeech::Speaking)
        m_tts->stop();

    // ── 3. Safety: destroy leftover audio objects from a previous session ──
    //   (should already be nullptr, but guards against edge cases where
    //    stopRecordingAndProcess was bypassed, e.g. rapid hide/show)
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    if (m_audioBuffer) {
        m_audioBuffer->close();
        delete m_audioBuffer;
        m_audioBuffer = nullptr;
    }

    // ── 4. Create fresh audio objects every time ───────────────────────────
    QAudioFormat fmt;
    fmt.setSampleRate(16000);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    m_audioSource = new QAudioSource(fmt, this);
    m_audioBuffer = new QBuffer(this);
    m_audioBuffer->open(QIODevice::WriteOnly);
    m_audioSource->start(m_audioBuffer);

    // ── 5. Detect silent failure (device busy / unavailable) ──────────────
    if (m_audioSource->state() == QAudio::StoppedState &&
        m_audioSource->error() != QAudio::NoError)
    {
        setStatus("❌ Microphone inaccessible. Réessayez.", true);
        delete m_audioSource; m_audioSource = nullptr;
        m_audioBuffer->close();
        delete m_audioBuffer; m_audioBuffer = nullptr;
        m_micBtn->setChecked(false);
        return;
    }

    m_isRecording = true;
    m_micBtn->setChecked(true);
    m_micBtn->setText("⏹");
    m_transcriptLbl->hide();
    m_actionLbl->hide();
    setStatus("🔴 Écoute... (cliquez pour arrêter)");
    m_pulseTimer->start();
    update();
}

void VoiceCommand::stopRecordingAndProcess()
{
    if (!m_isRecording) return;
    m_isRecording = false;
    m_micBtn->setChecked(false);
    m_micBtn->setText("🎤");
    m_pulseTimer->stop();
    update();

    // Stop source first, then grab PCM data, then DELETE synchronously
    // (NOT deleteLater) so the OS audio device handle is released immediately.
    QByteArray pcm;
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
    }
    if (m_audioBuffer) {
        pcm = m_audioBuffer->data();
        m_audioBuffer->close();
        delete m_audioBuffer;
        m_audioBuffer = nullptr;
    }

    if (pcm.size() < 2048) { setStatus("⚠ Trop court. Réessayez.", true); return; }
    setStatus("⏳ Transcription...");
    callGroqStt(pcm);
}

// ─── STT ──────────────────────────────────────────────────────────
void VoiceCommand::callGroqStt(const QByteArray& pcmData)
{
    const QByteArray wav = buildWav(pcmData, 16000, 1);
    QUrl url(GROQ_STT_API_URL);
    QNetworkRequest req(url);
    req.setRawHeader("Authorization", ("Bearer " + GROQ_API_KEY).toUtf8());
    req.setRawHeader("Connection", "close");  // no keep-alive reuse
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyNone);
    req.setSslConfiguration(ssl);

    QHttpMultiPart* mp = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    auto part = [](const QString& name, const QByteArray& val) {
        QHttpPart p;
        p.setHeader(QNetworkRequest::ContentDispositionHeader,
                    QString("form-data; name=\"%1\"").arg(name));
        p.setBody(val);
        return p;
    };
    mp->append(part("model",           GROQ_STT_API_MODEL.toUtf8()));
    mp->append(part("language",        "fr"));
    mp->append(part("response_format", "json"));
    mp->append(part("temperature",     "0"));
    // Vocabulaire métier : aide Whisper à mieux reconnaître les termes de l'application
    mp->append(part("prompt",
        "SmartVision, BioSample, biosample, congélateur, expérience, équipement, projet, "
        "publication, employé, laboratoire, échantillon, statistiques, ajouter, modifier, "
        "supprimer, effacer, détails, exporter, PDF, rechercher, actualiser, enregistrer, "
        "annuler, déconnecter, chatbot, assistant IA, freezer, ouvrir, fermer, naviguer."));

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "audio/wav");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       "form-data; name=\"file\"; filename=\"cmd.wav\"");
    filePart.setBody(wav);
    mp->append(filePart);

    QNetworkReply* rpl = m_net->post(req, mp);
    mp->setParent(rpl);
    m_pendingReply = rpl;       // track so we can abort it if needed
    rpl->setProperty("reqType", "stt");
    rpl->ignoreSslErrors();
    connect(rpl, &QNetworkReply::sslErrors, rpl,
            [rpl](const QList<QSslError>&){ rpl->ignoreSslErrors(); });
}

// ─── Network reply (STT only — CMD is now parsed locally) ─────────
void VoiceCommand::onNetworkReply(QNetworkReply* reply)
{
    // Clear the pending-reply tracker if this is the one we were waiting for
    if (reply == m_pendingReply)
        m_pendingReply = nullptr;

    // Discard replies that were aborted (e.g. user started a new recording
    // before the previous STT result came back, or widget was hidden).
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        reply->deleteLater();
        return;
    }

    const QByteArray data   = reply->readAll();
    const bool       netErr = (reply->error() != QNetworkReply::NoError);
    reply->deleteLater();

    if (netErr) { setStatus("❌ Erreur réseau STT.", true); return; }

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) { setStatus("❌ STT invalide.", true); return; }
    if (doc.object().contains("error")) {
        setStatus("❌ " + doc.object()["error"].toObject()["message"].toString(), true);
        return;
    }

    const QString text = doc.object()["text"].toString().trimmed();
    if (text.isEmpty()) { setStatus("⚠ Rien compris. Réessayez.", true); return; }

    m_transcriptLbl->setText("\"" + text + "\"");
    m_transcriptLbl->show();

    // ── Parse command locally — instant, no network ───────────────
    const Cmd parsed = parseCmd(text, m_contextName);

    if (parsed.action.isEmpty()) {
        setStatus("⚠ Commande non reconnue.", true);
        return;
    }

    QVariantMap params;
    params["description"] = parsed.desc;
    if (!parsed.searchText.isEmpty())
        params["text"] = parsed.searchText;

    if (!voiceModuleAllowedForRole(m_currentRole, parsed.module)) {
        const QString denied = "Accès refusé : ce module n'est pas autorisé pour votre rôle.";
        m_actionLbl->setText("⛔ " + denied);
        m_actionLbl->show();
        setStatus(denied, true);
        speak("Accès refusé.");
        QTimer::singleShot(3500, this, [this](){
            m_actionLbl->hide();
            m_transcriptLbl->hide();
            setStatus("Appuyez pour parler");
        });
        return;
    }

    m_actionLbl->setText("✅ " + parsed.desc);
    m_actionLbl->show();
    setStatus("✔ Commande exécutée !");
    speak(parsed.desc);
    emit commandExecute(parsed.action, parsed.module, params);

    QTimer::singleShot(3500, this, [this](){
        m_actionLbl->hide();
        m_transcriptLbl->hide();
        setStatus("Appuyez pour parler");
    });
}

// ─── Helpers ──────────────────────────────────────────────────────
void VoiceCommand::setStatus(const QString& text, bool isError)
{
    m_statusLbl->setText(text);
    m_statusLbl->setStyleSheet(
        isError
        ? "color:rgba(252,165,165,0.90); font-size:11px; font-weight:700; background:transparent; padding:2px;"
        : "color:rgba(200,210,255,0.85); font-size:11px; font-weight:700; background:transparent; padding:2px;"
    );
}

void VoiceCommand::speak(const QString& text)
{
    if (!m_tts) return;
    if (m_tts->state() == QTextToSpeech::Speaking) m_tts->stop();
    m_tts->say(text);
}
