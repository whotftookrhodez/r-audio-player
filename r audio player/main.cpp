#include <QApplication>
#include <QCoreApplication>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QLockFile>
#include <QScreen>
#include <QGuiApplication>

#include "settings.h"
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationName("r audio player");
    QCoreApplication::setOrganizationName("r");
    QCoreApplication::setOrganizationDomain("github.com/whotftookrhodez/raudioplayer");

    const QIcon appIcon(":/r audio player.ico");

    app.setWindowIcon(appIcon);

    const QString lockPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QDir().mkpath(lockPath);

    QLockFile lock(lockPath + "/instance.lock");

    lock.setStaleLockTime(0);

    if (!lock.tryLock(0)) {
        return 0;
    }

    Settings settings;

    settings.load();

    MainWindow window(&settings);

    window.setWindowIcon(appIcon);
    window.setMinimumSize(640, 480);

    QScreen* screen = QGuiApplication::primaryScreen();

    if (screen) {
        QRect available = screen->availableGeometry();
        QSize halfSize(available.width() / 2,
            available.height() / 2);

        window.resize(halfSize);
        window.move(available.center() - window.rect().center());
    }

    window.show();

    const int result = app.exec();

    settings.save();

    return result;
}