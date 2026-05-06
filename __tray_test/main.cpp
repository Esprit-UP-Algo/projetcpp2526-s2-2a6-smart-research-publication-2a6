#include <QApplication>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QTimer>
#include <QStyle>
#include <QMessageBox>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(nullptr, "Test", "System tray NON disponible sur cette machine.");
        return 1;
    }

    QSystemTrayIcon tray;
    tray.setIcon(app.style()->standardIcon(QStyle::SP_ComputerIcon));
    tray.setToolTip("SmartVision BioSimple — Test");
    tray.show();

    // Notification critique (maintenance EN RETARD)
    QTimer::singleShot(800, [&]() {
        tray.showMessage(
            "⚠ Maintenance EN RETARD — SmartVision",
            "Centrifugeuse C-400, Microscope BX53",
            QSystemTrayIcon::Critical,
            6000
        );
    });

    // Notification warning (imminente) après 4s
    QTimer::singleShot(4000, [&]() {
        tray.showMessage(
            "🔔 Maintenance imminente — SmartVision",
            "Autoclave A-200 (dans 3 jours)",
            QSystemTrayIcon::Warning,
            6000
        );
    });

    // Quitter après 10s
    QTimer::singleShot(10000, &app, &QApplication::quit);

    return app.exec();
}
