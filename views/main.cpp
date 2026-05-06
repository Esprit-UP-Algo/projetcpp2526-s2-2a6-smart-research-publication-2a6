#include "integration.h"    // previously biosimple.h
#include "connection.h"
#include <QApplication>
#include <QMessageBox>
#include <QFont>

// =========================================================
// GLOBAL QSS — SmartVision Dark Teal (exact screenshot match)
// =========================================================
static const char* kGlobalQss = R"QSS(
/* ── Base ──────────────────────────────────────────────── */
QMainWindow, QWidget {
    background: transparent;
    color: #EAFBFF;
    font-family: "Segoe UI", "Arial";
    font-size: 13px;
}
QMainWindow { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #062B31,stop:1 #03080D); }

/* ── Scroll areas ──────────────────────────────────────── */
QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: none; }

/* ── Labels ────────────────────────────────────────────── */
QLabel { color: #EAFBFF; background: transparent; }

/* ── Line edit ─────────────────────────────────────────── */
QLineEdit {
    background: rgba(4,30,36,0.80);
    border: 1px solid rgba(0,240,200,0.32);
    border-radius: 10px;
    padding: 7px 12px;
    color: #EAFBFF;
    font-size: 13px;
    min-height: 18px;
}
QLineEdit:focus {
    border: 1px solid rgba(0,240,200,0.85);
    background: rgba(0,50,60,0.88);
}
QLineEdit:hover { border-color: rgba(0,240,200,0.55); }

/* ── ComboBox ──────────────────────────────────────────── */
QComboBox {
    background: rgba(4,30,36,0.80);
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 8px;
    padding: 6px 10px 6px 10px;
    color: #EAFBFF;
    font-size: 13px;
    font-weight: 700;
    min-width: 70px;
    min-height: 18px;
}
QComboBox:hover { border-color: rgba(0,240,200,0.60); }
QComboBox:focus { border-color: rgba(0,240,200,0.85); }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background: #062B31;
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 8px;
    color: #EAFBFF;
    selection-background-color: rgba(0,191,166,0.30);
    padding: 3px;
    outline: none;
}

/* ── Table ─────────────────────────────────────────────── */
QTableWidget {
    background: rgba(3,18,22,0.90);
    alternate-background-color: rgba(6,43,49,0.85);
    border: 1px solid rgba(0,240,200,0.15);
    border-radius: 12px;
    gridline-color: rgba(0,240,200,0.08);
    color: #EAFBFF;
    selection-background-color: rgba(0,191,166,0.28);
    selection-color: #EAFBFF;
    outline: none;
}
QTableWidget::item {
    padding: 6px 8px;
    border: none;
    color: #EAFBFF;
}
QTableWidget::item:hover  { background: rgba(0,240,200,0.07); }
QTableWidget::item:selected { background: rgba(0,191,166,0.28); color: #EAFBFF; }

QHeaderView::section {
    background: rgba(6,43,49,0.95);
    color: rgba(0,240,200,0.88);
    border: none;
    border-bottom: 1px solid rgba(0,240,200,0.22);
    padding: 5px 8px;
    font-weight: 700;
    font-size: 11px;
}
QHeaderView { background: transparent; }

/* ── Scroll bars ───────────────────────────────────────── */
QScrollBar:vertical {
    background: rgba(0,30,36,0.35);
    width: 5px;
    border-radius: 3px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: rgba(0,240,200,0.38);
    border-radius: 3px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover { background: rgba(0,240,200,0.70); }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
QScrollBar:horizontal {
    background: rgba(0,30,36,0.35);
    height: 5px;
    border-radius: 3px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: rgba(0,240,200,0.38);
    border-radius: 3px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover { background: rgba(0,240,200,0.70); }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── Frames (glass cards) ──────────────────────────────── */
QFrame { border-radius: 10px; }

/* ── Checkboxes ────────────────────────────────────────── */
QCheckBox { color: #EAFBFF; spacing: 6px; background: transparent; }
QCheckBox::indicator {
    width: 16px; height: 16px;
    border: 1px solid rgba(0,240,200,0.38);
    border-radius: 4px;
    background: rgba(4,30,36,0.80);
}
QCheckBox::indicator:checked {
    background: rgba(0,191,166,0.72);
    border-color: rgba(0,240,200,0.72);
}

/* ── Spin / Date ───────────────────────────────────────── */
QSpinBox, QDoubleSpinBox, QDateEdit {
    background: rgba(4,30,36,0.80);
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 8px;
    padding: 6px 10px;
    color: #EAFBFF;
    min-height: 18px;
}
QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus { border-color: rgba(0,240,200,0.85); }
QSpinBox::up-button, QSpinBox::down-button,
QDoubleSpinBox::up-button, QDoubleSpinBox::down-button {
    background: rgba(0,191,166,0.25);
    border: none;
    border-radius: 3px;
    width: 14px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background: rgba(0,191,166,0.50);
}

/* ── Text edit ─────────────────────────────────────────── */
QTextEdit {
    background: rgba(4,30,36,0.80);
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 8px;
    padding: 7px;
    color: #EAFBFF;
}
QTextEdit:focus { border-color: rgba(0,240,200,0.85); }

/* ── Tree widget ───────────────────────────────────────── */
QTreeWidget {
    background: transparent;
    border: none;
    color: #EAFBFF;
    outline: none;
}
QTreeWidget::item { padding: 5px; border-radius: 6px; color: #EAFBFF; }
QTreeWidget::item:selected { background: rgba(0,191,166,0.28); }
QTreeWidget::item:hover { background: rgba(0,240,200,0.08); }
QTreeWidget::branch { background: transparent; }

/* ── List widget ───────────────────────────────────────── */
QListWidget { background: transparent; border: none; color: #EAFBFF; outline: none; }
QListWidget::item { color: #EAFBFF; padding: 5px; border-radius: 6px; }
QListWidget::item:selected { background: rgba(0,191,166,0.28); }
QListWidget::item:hover { background: rgba(0,240,200,0.07); }

/* ── Tool button ───────────────────────────────────────── */
QToolButton {
    color: rgba(0,240,200,0.82);
    padding: 5px;
    border-radius: 8px;
    background: transparent;
    border: none;
}
QToolButton:hover { background: rgba(0,240,200,0.12); color: #00F0C8; }

/* ── Push button base ──────────────────────────────────── */
QPushButton {
    background: rgba(4,30,36,0.80);
    color: #EAFBFF;
    border: 1px solid rgba(0,240,200,0.32);
    border-radius: 10px;
    padding: 7px 14px;
    font-weight: 700;
    font-size: 13px;
    min-height: 18px;
}
QPushButton:hover { border-color: rgba(0,240,200,0.70); background: rgba(0,50,60,0.85); }
QPushButton:pressed { background: rgba(0,60,70,0.90); }
QPushButton:disabled { background: rgba(10,40,46,0.50); color: rgba(180,220,218,0.35); border-color: rgba(0,240,200,0.10); }

/* ── Group box ─────────────────────────────────────────── */
QGroupBox {
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 10px;
    margin-top: 10px;
    padding-top: 6px;
    color: #EAFBFF;
    font-weight: 700;
    background: transparent;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 10px;
    color: rgba(0,240,200,0.85);
    background: transparent;
}

/* ── Tab bar ───────────────────────────────────────────── */
QTabWidget::pane {
    border: 1px solid rgba(0,240,200,0.28);
    border-radius: 8px;
    background: rgba(4,30,36,0.80);
}
QTabBar::tab {
    background: rgba(4,30,36,0.80);
    border: 1px solid rgba(0,240,200,0.22);
    border-radius: 6px;
    padding: 6px 12px;
    color: rgba(180,230,225,0.75);
    margin-right: 3px;
    font-weight: 700;
}
QTabBar::tab:selected {
    background: rgba(0,160,130,0.55);
    color: #EAFBFF;
    border-color: rgba(0,240,200,0.55);
}
QTabBar::tab:hover { background: rgba(0,240,200,0.10); color: #EAFBFF; }

/* ── Message / Dialog ──────────────────────────────────── */
QMessageBox, QDialog {
    background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #062B31,stop:1 #03080D);
    color: #EAFBFF;
}
QMessageBox QPushButton, QDialog QPushButton {
    background: rgba(0,160,130,0.55);
    color: white;
    border: 1px solid rgba(0,240,200,0.45);
    border-radius: 8px;
    padding: 6px 16px;
    font-weight: 700;
    min-width: 70px;
}
QMessageBox QPushButton:hover, QDialog QPushButton:hover {
    background: rgba(0,200,165,0.70);
    border-color: rgba(0,240,200,0.80);
}

/* ── Calendar ──────────────────────────────────────────── */
QCalendarWidget {
    background: #062B31;
    color: #EAFBFF;
    border: 1px solid rgba(0,240,200,0.30);
    border-radius: 10px;
}
QCalendarWidget QAbstractItemView {
    background: #062B31;
    color: #EAFBFF;
    selection-background-color: rgba(0,191,166,0.40);
}
)QSS";

int main(int argc, char *argv[])
{
    // Force software decode to avoid repeated d3d11 hwaccel failures on some GPUs/drivers.
    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "");

    QApplication a(argc, argv);

    // ── Global stylesheet (covers all Qt widgets app-wide) ──
    a.setStyleSheet(kGlobalQss);

    // ── Default font ──
    QFont appFont("Segoe UI", 10);
    a.setFont(appFont);

    Connection* c = Connection::instance();
    bool test=c->createConnect();
    MainWindow w;
    if(test)
    {w.show();
        QMessageBox::information(nullptr, QObject::tr("database is open"),
                                 QObject::tr("connection successful.\n"
                                             "Click Cancel to exit."), QMessageBox::Cancel);

    }
    else
        QMessageBox::critical(nullptr, QObject::tr("database is not open"),
                              QObject::tr("connection failed.\n"
                                          "Click Cancel to exit."), QMessageBox::Cancel);



    return a.exec();
}

