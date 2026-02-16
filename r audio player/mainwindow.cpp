// includes are alphabetical in files > 1000 lines

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QMovie>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QShortcut>
#include <QStorageInfo>
#include <QStringList>
#include <QVBoxLayout>

#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>
#include <taglib/tbytevector.h>
#include <taglib/tpropertymap.h>
#include <taglib/vorbisfile.h>
#include <taglib/xiphcomment.h>

#include "clickslider.h"
#include "folderdialog.h"
#include "mainwindow.h"
#include "settingsdialog.h"

namespace
{
    struct ImageBackgroundFilter final : QObject
    {
        QWidget* host = nullptr;
        bool fill = false;

        QPixmap source;
        QPixmap cached;
        QSize lastTargetPx;

        ImageBackgroundFilter(QWidget* h, const QString& path, bool fillBackground)
            :
            QObject(h),
            host(h),
            fill(fillBackground)
        {
            setObjectName("__img_bg_filter");

            if (!path.isEmpty()
                && QFileInfo::exists(path))
            {
                QPixmap p;
                p.load(path);

                source = p;
            }

            rebuildCache();

            if (host)
            {
                host->update();
            }
        }

        void updateFill(bool v)
        {
            if (fill == v)
            {
                return;
            }

            fill = v;

            cached = QPixmap();
            lastTargetPx = QSize();

            rebuildCache();

            if (host)
            {
                host->update();
            }
        }

        void rebuildCache()
        {
            if (!host)
            {
                return;
            }

            if (source.isNull())
            {
                cached = QPixmap();
                lastTargetPx = QSize();

                return;
            }

            const qreal dpr = host->devicePixelRatioF();

            const QSize targetPx(
                int(std::round(host->width() * dpr)),
                int(std::round(host->height() * dpr))
            );

            if (targetPx.isEmpty()
                || targetPx == lastTargetPx)
            {
                return;
            }

            lastTargetPx = targetPx;

            const int dw = targetPx.width();
            const int dh = targetPx.height();

            if (!fill)
            {
                QPixmap scaled = source.scaled(
                    targetPx,
                    Qt::IgnoreAspectRatio,
                    Qt::SmoothTransformation
                );

                scaled.setDevicePixelRatio(dpr);

                cached = scaled;

                return;
            }

            const int sw = source.width();
            const int sh = source.height();

            if (sw <= 0
                || sh <= 0
                || dw <= 0
                || dh <= 0)
            {
                cached = QPixmap();

                return;
            }

            QRect src;

            if (sw * dh > dw * sh)
            {
                const int cw = sh * dw / dh;
                const int x = (sw - cw) / 2;

                src = QRect(x, 0, cw, sh);
            }
            else
            {
                const int ch = sw * dh / dw;
                const int y = (sh - ch) / 2;

                src = QRect(0, y, sw, ch);
            }

            QPixmap cropped = source.copy(src);

            QPixmap scaled = cropped.scaled(
                targetPx,
                Qt::IgnoreAspectRatio,
                Qt::SmoothTransformation
            );

            scaled.setDevicePixelRatio(dpr);

            cached = scaled;
        }

        bool eventFilter(QObject* obj, QEvent* e) override
        {
            if (!host
                || obj != host)
            {
                return QObject::eventFilter(
                    obj,
                    e
                );
            }

            switch (e->type())
            {
            case QEvent::Show:
            case QEvent::Resize:
            case QEvent::WindowStateChange:
                rebuildCache();

                break;
            case QEvent::Paint:
            {
                if (cached.isNull())
                {
                    break;
                }

                auto* pe = static_cast<QPaintEvent*>(e);

                QPainter p(host);
                p.drawPixmap(
                    0,
                    0,
                    cached
                );

                break;
            }

            default:
                break;
            }

            return QObject::eventFilter(
                obj,
                e
            );
        }
    };

    static void updateImageBackground(QWidget* host, const QString& path, bool fillBackground)
    {
        const QObjectList children = host->children();

        for (QObject* o : children)
        {
            if (!o)
            {
                continue;
            }

            if (o->objectName() == "__img_bg_filter")
            {
                auto* f = static_cast<ImageBackgroundFilter*>(o);

                host->removeEventFilter(f);

                f->deleteLater();
            }
        }

        if (path.isEmpty()
            || !QFileInfo::exists(path))
        {
            host->update();

            return;
        }

        auto* filter = new ImageBackgroundFilter(host, path, fillBackground);

        host->installEventFilter(filter);
        host->update();
    }
}

namespace
{
    struct GifBackgroundFilter final : QObject
    {
        QWidget* host = nullptr;
        QMovie movie;
        QSize lastScaled;
        bool fill = false;
        QSize lastHostSize;
        QSize lastFrameSize;
        QRect cachedSrc;
        QRect cachedDst;

        GifBackgroundFilter(QWidget* h, const QString& path, bool fillBackground)
            :
            QObject(h),
            host(h),
            movie(path),
            fill(fillBackground)
        {
            setObjectName("__gif_bg_filter");
            updateScale();

            movie.start();

            QObject::connect(
                &movie,
                &QMovie::frameChanged,
                this,
                [this]
                {
                    updateGeometry();

                    if (host)
                    {
                        host->update();
                    }
                }
            );
        }

        void updateGeometry()
        {
            if (!host
                || !movie.isValid())
            {
                return;
            }

            const QSize hostSize = host->size();

            if (hostSize.isEmpty())
            {
                return;
            }

            const QSize frameSize = movie.currentImage().size();

            if (hostSize == lastHostSize
                && frameSize == lastFrameSize)
            {
                return;
            }

            lastHostSize = hostSize;
            lastFrameSize = frameSize;

            cachedDst = QRect(
                QPoint(
                    0,
                    0
                ),

                hostSize
            );

            if (!fill)
            {
                cachedSrc = QRect(
                    QPoint(
                        0,
                        0
                    ),

                    frameSize
                );

                return;
            }

            const int sw = frameSize.width();
            const int sh = frameSize.height();
            const int dw = hostSize.width();
            const int dh = hostSize.height();

            if (sw <= 0
                || sh <= 0
                || dw <= 0
                || dh <= 0)
            {
                return;
            }

            if (sw * dh > dw * sh)
            {
                const int cw = sh * dw / dh;
                const int x = (sw - cw) / 2;

                cachedSrc = QRect(
                    x,
                    0,
                    cw,
                    sh
                );
            }
            else
            {
                const int ch = sw * dh / dw;
                const int y = (sh - ch) / 2;

                cachedSrc = QRect(
                    0,
                    y,
                    sw,
                    ch
                );
            }
        }

        void updateFill(bool v)
        {
            if (fill == v)
            {
                return;
            }

            fill = v;

            lastScaled = QSize();
            lastHostSize = QSize();
            lastFrameSize = QSize();

            updateScale();
            updateGeometry();

            if (host)
            {
                host->update();
            }
        }

        void updateScale()
        {
            if (!host)
            {
                return;
            }

            if (fill)
            {
                movie.setScaledSize(QSize());

                return;
            }

            const qreal dpr = host->devicePixelRatioF();

            const QSize want(
                int(std::round(host->width() * dpr)),
                int(std::round(host->height() * dpr))
            );

            if (want.isEmpty()
                || want == lastScaled)
            {
                return;
            }

            lastScaled = want;

            movie.setScaledSize(want);
        }

        void updateMovieState()
        {
            if (!host)
            {
                return;
            }

            const bool start = host->isVisible()
                && !host->isMinimized();

            if (start)
            {
                if (movie.state() != QMovie::Running)
                {
                    movie.start();
                }
            }
            else if (movie.state() != QMovie::NotRunning)
            {
                movie.stop();
            }
        }

        bool eventFilter(QObject* obj, QEvent* e) override
        {
            if (!host
                || obj != host)
            {
                return QObject::eventFilter(
                    obj,
                    e
                );
            }

            switch (e->type())
            {
            case QEvent::Show:
            case QEvent::Hide:
            case QEvent::WindowStateChange:
                updateMovieState();

                break;
            case QEvent::Resize:
                updateScale();
                updateGeometry();

                break;
            case QEvent::Paint:
            {
                if (!movie.isValid()
                    || movie.state() != QMovie::Running)
                {
                    break;
                }

                QPainter p(host);

                const QPixmap frame = movie.currentPixmap();

                if (frame.isNull())
                {
                    break;
                }

                if (!fill)
                {
                    p.drawPixmap(
                        0,
                        0,
                        frame
                    );
                }
                else
                {
                    p.drawPixmap(
                        cachedDst,
                        frame,
                        cachedSrc
                    );
                }

                break;
            }

            default:
                break;
            }

            return QObject::eventFilter(
                obj,
                e
            );
        }
    };

    static void updateAnimatedBackground(QWidget* host, const QString& path, bool fillBackground)
    {
        const QObjectList children = host->children();

        for (QObject* o : children)
        {
            if (!o)
            {
                continue;
            }

            if (o->objectName() == "__gif_bg_filter")
            {
                auto* f = static_cast<GifBackgroundFilter*>(o);

                host->removeEventFilter(f);

                f->deleteLater();
            }
        }

        if (path.isEmpty()
            || !QFileInfo::exists(path))
        {
            host->update();

            return;
        }

        auto* filter = new GifBackgroundFilter(
            host,
            path,
            fillBackground
        );

        host->installEventFilter(filter);
        host->update();
    }
}

static QString qs(const std::string& s)
{
    return QString::fromUtf8(s.data(), int(s.size()));
}

static QString qs(const std::wstring& w)
{
    return QString::fromStdWString(w);
}

static void showCoverFullscreen(const QPixmap& pix, bool pip, const QString& title, const QWidget* centerOn)
{
    if (pix.isNull())
    {
        return;
    }

    auto* dlg = new QDialog(
        nullptr,
        pip
        ? (Qt::WindowStaysOnTopHint | Qt::Window)
        : (Qt::FramelessWindowHint | Qt::Window)
    );

    dlg->setAttribute(Qt::WA_DeleteOnClose);

    if (pip)
    {
        dlg->resize(480, 480);
        dlg->move(centerOn->frameGeometry().center() - dlg->rect().center());
        dlg->setWindowTitle(title);
    }
    else
    {
        QPoint cursorPos = QCursor::pos();
        QScreen* screen = QGuiApplication::screenAt(cursorPos);

        if (!screen)
        {
            screen = QGuiApplication::primaryScreen();
        }

        if (screen)
        {
            dlg->setGeometry(screen->availableGeometry());
        }
    }

    auto* label = new QLabel(dlg);
    label->setAlignment(Qt::AlignCenter);

    if (pip)
    {
        label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    }

    auto* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);

    struct Filter final : QObject
    {
        QPixmap pix;
        QLabel* label = nullptr;
        QDialog* dlg = nullptr;

        bool pip;

        void rescale()
        {
            if (!label
                || pix.isNull())
            {
                return;
            }

            const QSize s = label->size();

            if (s.isEmpty())
            {
                return;
            }

            label->setPixmap(
                pix.scaled(
                    s,
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation
                )
            );
        }

        bool eventFilter(QObject* obj, QEvent* e) override
        {
            switch (e->type())
            {
            case QEvent::Show:
            case QEvent::Resize:
            case QEvent::WindowStateChange:
                rescale();

                break;
            case QEvent::MouseButtonPress:
                if (!pip && dlg)
                {
                    dlg->close();

                    return true;
                }

                break;
            case QEvent::KeyPress:
            {
                auto* ke = static_cast<QKeyEvent*>(e);

                if (ke->key() == Qt::Key_Escape
                    && dlg)
                {
                    dlg->close();

                    return true;
                }

                break;
            }

            default:
                break;
            }

            return QObject::eventFilter(
                obj,
                e
            );
        }
    };

    auto* f = new Filter;
    f->pix = pix;
    f->label = label;
    f->dlg = dlg;
    f->pip = pip;

    f->setParent(dlg);

    label->installEventFilter(f);
    dlg->installEventFilter(f);

    dlg->show();

    f->rescale();
}

static QSet<QString> getMountedRootSet()
{
    QSet<QString> roots;

    const auto volumes = QStorageInfo::mountedVolumes();

    for (const QStorageInfo& v : volumes)
    {
        if (!v.isValid()
            || !v.isReady())
        {
            continue;
        }

        const QString root = QDir::cleanPath(v.rootPath());

        if (root.isEmpty())
        {
            continue;
        }

        roots.insert(root);
    }

    return roots;
}

static QString mountRootForPath(const QString& p)
{
    const QString clean = QDir::cleanPath(p);

    QStorageInfo si(clean);

    if (si.isValid()
        && !si.rootPath().isEmpty())
    {
        return QDir::cleanPath(si.rootPath());
    }

    // proprietary ugliness
#ifdef _WIN32
    if (clean.size() >= 2
        && clean[1] == QChar(':'))
    {
        QString driveRoot;

        driveRoot += clean[0];
        driveRoot += ":/";

        QStorageInfo si2(driveRoot);

        if (si2.isValid()
            && !si2.rootPath().isEmpty())
        {
            return QDir::cleanPath(si2.rootPath());
        }

        return QDir::cleanPath(driveRoot);
    }
#endif

    return {};
}

int MainWindow::viewedAlbumIndex() const
{
    if (auto* ai = albums->currentItem())
    {
        return ai->data(Qt::UserRole).toInt();
    }

    return -1;
}

int MainWindow::visibleRowForTrackIndex(int trackIndex) const
{
    for (int i = 0; i < int(searchTrackOrder.size()); ++i)
    {
        if (searchTrackOrder[i] == trackIndex)
        {
            return i;
        }
    }

    return -1;
}

void MainWindow::lastfmUpdateNowPlaying(const Track& t)
{
    if (settings->lastfmSessionKey.isEmpty())
    {
        return;
    }

    if (curAlbum < 0 || curAlbum >= library.getAlbums().size())
    {
        return;
    }

    const auto& album = library.getAlbums()[curAlbum];

    QUrl url("https://ws.audioscrobbler.com/2.0/");

    QNetworkRequest req(url);

    req.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/x-www-form-urlencoded"
    );

    QUrlQuery query;

    query.addQueryItem(
        "method",
        "track.updateNowPlaying"
    );

    query.addQueryItem(
        "track",
        qs(t.title)
    );

    query.addQueryItem(
        "artist",
        album.variousArtists
        ? qs(t.artists.front())
        : qs(album.artists.front())
    );

    query.addQueryItem(
        "album",
        qs(album.title)
    );

    query.addQueryItem(
        "sk",
        settings->lastfmSessionKey
    );

    query.addQueryItem(
        "api_key",
        Settings::LASTFM_API_KEY
    );

    QString sigBase = "album" + qs(album.title)
        + "api_key" + Settings::LASTFM_API_KEY
        + "artist" + (album.variousArtists
            ? qs(t.artists.front())
            : qs(album.artists.front()))
        + "methodtrack.updateNowPlaying"
        + "sk" + settings->lastfmSessionKey
        + "track" + qs(t.title)
        + Settings::LASTFM_API_SECRET;

    query.addQueryItem(
        "api_sig",
        QCryptographicHash::hash(
            sigBase.toUtf8(),
            QCryptographicHash::Md5
        ).toHex()
    );

    query.addQueryItem(
        "format",
        "json"
    );

    nam->post(
        req,
        query.query(QUrl::FullyEncoded).toUtf8()
    );
}

void MainWindow::lastfmScrobbleTrack(const Track& t)
{
    if (settings->lastfmSessionKey.isEmpty())
    {
        return;
    }

    if (curAlbum < 0 || curAlbum >= library.getAlbums().size())
    {
        return;
    }

    const auto& album = library.getAlbums()[curAlbum];

    QUrl url("https://ws.audioscrobbler.com/2.0/");

    QNetworkRequest req(url);

    req.setHeader(
        QNetworkRequest::ContentTypeHeader,
        "application/x-www-form-urlencoded"
    );

    QUrlQuery query;

    qint64 ts = QDateTime::currentSecsSinceEpoch();

    query.addQueryItem(
        "method",
        "track.scrobble"
    );

    query.addQueryItem(
        "track",
        qs(t.title)
    );

    query.addQueryItem(
        "artist",
        album.variousArtists
        ? qs(t.artists.front())
        : qs(album.artists.front())
    );

    query.addQueryItem(
        "album",
        qs(album.title)
    );

    query.addQueryItem(
        "timestamp",
        QString::number(ts)
    );

    query.addQueryItem(
        "sk",
        settings->lastfmSessionKey
    );

    query.addQueryItem(
        "api_key",
        Settings::LASTFM_API_KEY
    );

    QString sigBase = "album" + qs(album.title)
        + "api_key" + Settings::LASTFM_API_KEY
        + "artist" + (album.variousArtists
            ? qs(t.artists.front())
            : qs(album.artists.front()))
        + "methodtrack.scrobble"
        + "sk" + settings->lastfmSessionKey
        + "timestamp" + QString::number(ts)
        + "track" + qs(t.title)
        + Settings::LASTFM_API_SECRET;

    query.addQueryItem(
        "api_sig",
        QCryptographicHash::hash(
            sigBase.toUtf8(),
            QCryptographicHash::Md5
        ).toHex()
    );

    query.addQueryItem(
        "format",
        "json"
    );

    nam->post(
        req,
        query.query(QUrl::FullyEncoded).toUtf8()
    );
}

MainWindow::MainWindow(Settings* s) : settings(s)
{
    // definitely a few redundancies in the stylesheet
    // throwing in it is safe (verify with Terminal=true)
    // plus they're convenient for adding later
    // thus don't bother
    qApp->setStyleSheet(R"(
        QWidget
        {
            background-color: #1a1a1a;
            color: #e6e6e6;
        }

        QMainWindow
        {
            background-color: #1a1a1a;
        }

        QDialog
        {
            background-color: #1a1a1a;
        }

        QLineEdit
        {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            padding: 6px;
            selection-background-color: #333333;
            selection-color: #e6e6e6;
        }

        QLineEdit:focus
        {
            outline: none;
            border: 1px solid #333333;
        }

        QLabel
        {
            background-color: #1a1a1a;
            color: #e6e6e6;
        }

        QPushButton
        {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            padding: 6px 10px;
        }

        QPushButton:hover
        {
            background-color: #333333;
        }

        QPushButton:pressed
        {
            background-color: #333333;
        }

        QPushButton:focus
        {
            outline: none;
        }

        QSpinBox
        {
            background-color: #1a1a1a;
            color: #e6e6e6;
            border: 1px solid #333333;
            padding: 6px;
            selection-background-color: #333333;
            selection-color: #e6e6e6;
        }

        QSpinBox::up-button,
        QSpinBox::down-button
        {
            subcontrol-origin: border;
            width: 18px;
            border-left: 1px solid #333333;
            background-color: #1a1a1a;
        }

        QSpinBox::up-button
        {
            subcontrol-position: top right;
            height: 50%;
            border-bottom: 1px solid #333333;
        }

        QSpinBox::down-button
        {
            subcontrol-position: bottom right;
            height: 50%;
        }

        QSpinBox::up-button:hover,
        QSpinBox::down-button:hover
        {
            background-color: #333333;
        }

        QSpinBox::up-button:pressed,
        QSpinBox::down-button:pressed
        {
            background-color: #333333;
        }

        QSpinBox::up-arrow,
        QSpinBox::down-arrow
        {
            width: 0px;
            height: 0px;
        }

        QCheckBox
        {
            spacing: 6px;
        }

        QCheckBox::indicator
        {
            width: 14px;
            height: 14px;
            border: 1px solid #333333;
            background-color: #1a1a1a;
        }

        QCheckBox::indicator:checked
        {
            background-color: #333333;
        }

        QCheckBox::indicator:focus
        {
            outline: none;
        }

        QListWidget
        {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            outline: none;
        }

        QListWidget::item
        {
            padding: 6px;
        }

        QListWidget::item:selected
        {
            background-color: #333333;
            color: #e6e6e6;
        }

        QListWidget::item:selected:!active
        {
            background-color: #333333;
        }

        QListWidget::item:hover {
            background-color: #333333;
        }

        QSlider
        {
            background-color: #1a1a1a;
        }

        QSlider::groove:horizontal
        {
            height: 6px;
            background-color: #333333;
            border: none;
        }

        QSlider::handle:horizontal
        {
            width: 12px;
            margin: -4px 0;
            background-color: #1a1a1a;
            border: 1px solid #333333;
        }

        QSlider::handle:horizontal:hover
        {
            background-color: #333333;
        }

        QSlider::sub-page:horizontal
        {
            background-color: #333333;
        }

        QSlider::add-page:horizontal
        {
            background-color: #1a1a1a;
        }

        QScrollBar:vertical,
        QScrollBar:horizontal
        {
            background-color: transparent;
            border: none;
        }

        QScrollBar::groove:vertical,
        QScrollBar::groove:horizontal
        {
            background: transparent;
            border: none;
        }

        QScrollBar::handle:vertical,
        QScrollBar::handle:horizontal
        {
            background-color: #333333;
            min-width: 20px;
            min-height: 20px;
        }

        QScrollBar::add-line,
        QScrollBar::sub-line
        {
            background: none;
            border: none;
        }

        QScrollBar::add-page,
        QScrollBar::sub-page
        {
            background: none;
        }

        QListView
        {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            outline: none;
        }

        QListView::item:selected
        {
            background-color: #333333;
        }

        *:focus
        {
            outline: none;
        }
    )");

    mainStyleSheet = qApp->styleSheet();

    setObjectName("mw");

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_StyledBackground, true);

    audio.init();
    audio.setVolume(settings->volume);

    if (settings->folders.isEmpty())
    {
        FolderDialog dlg(
            settings->folders,
            this
        );

        if (dlg.exec())
        {
            settings->folders = dlg.selectedFolders();
        }
    }

    std::vector<std::filesystem::path> roots;

    for (const auto& f : settings->folders)
    {
        roots.emplace_back(std::filesystem::path(f.toUtf8().constData()));
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);

    library.scan(roots);

    QApplication::restoreOverrideCursor();

    search = new QLineEdit(this);
    search->setPlaceholderText("search");
    search->setClearButtonEnabled(true);
    search->setMinimumHeight(28);

    albums = new QListWidget(this);
    tracks = new QListWidget(this);

    auto lists = new QHBoxLayout;
    lists->setSpacing(6);
    lists->addWidget(albums, 1);
    lists->addWidget(tracks, 2);

    coverLabel = new ClickLabel(this);
    coverLabel->setCursor(Qt::PointingHandCursor);
    coverLabel->setFixedHeight(settings->coverSize);
    coverLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    coverLabel->setScaledContents(false);
    coverLabel->hide();

    nowPlaying = new QLabel("nothing playing", this);
    nowPlaying->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    nowPlaying->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto nowPlayingLayout = new QHBoxLayout;
    nowPlayingLayout->setSpacing(6);
    nowPlayingLayout->setAlignment(Qt::AlignCenter);
    nowPlayingLayout->addWidget(coverLabel);
    nowPlayingLayout->addWidget(nowPlaying);

    auto nowPlayingContainer = new QWidget(this);
    nowPlayingContainer->setLayout(nowPlayingLayout);

    auto backwardButton = new QPushButton("", this);
    auto playPauseButton = new QPushButton("", this);
    auto forwardButton = new QPushButton("", this);

    iconButtonsList =
    {
        backwardButton,
        playPauseButton,
        forwardButton
    };

    backwardIcon = QIcon(":/backward.svg");
    playPauseIcon = QIcon(":/playpause.svg");
    forwardIcon = QIcon(":/forward.svg");

    updateControlsText();

    autoplay = new QCheckBox("autoplay", this);
    autoplay->setChecked(settings->autoplay);
    autoplay->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    autoplay->setFixedWidth(autoplay->sizeHint().width());

    cursorSlider = new ClickSlider(Qt::Horizontal, this);
    cursorSlider->setRange(0, 2147483647);

    cursorText = new QLabel("", this);
    cursorText->setAlignment(Qt::AlignVCenter | Qt::AlignRight);
    cursorText->setMinimumWidth(cursorText->fontMetrics().horizontalAdvance(""));

    volumeSlider = new ClickSlider(Qt::Horizontal, this);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(int(settings->volume * 100));

    const int h = backwardButton->sizeHint().height();

    cursorSlider->setMinimumHeight(h);
    volumeSlider->setMinimumHeight(h);

    auto settingsButton = new QPushButton("settings", this);

    auto controls = new QHBoxLayout;
    controls->setSpacing(6);
    controls->addWidget(backwardButton);
    controls->addWidget(playPauseButton);
    controls->addWidget(forwardButton);
    controls->addWidget(autoplay, 0, Qt::AlignHCenter);
    controls->addWidget(cursorSlider, 1);
    controls->addWidget(cursorText);
    controls->addWidget(volumeSlider);
    controls->addWidget(settingsButton);

    auto root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->addWidget(search);
    root->addLayout(lists);
    root->addWidget(nowPlayingContainer);
    root->addLayout(controls);

    search->setObjectName("search");
    albums->setObjectName("albums");
    tracks->setObjectName("tracks");
    nowPlaying->setObjectName("nowPlaying");
    nowPlayingContainer->setObjectName("nowPlayingContainer");
    backwardButton->setObjectName("backwardButton");
    playPauseButton->setObjectName("playPauseButton");
    forwardButton->setObjectName("forwardButton");
    autoplay->setObjectName("autoplay");
    cursorSlider->setObjectName("cursorSlider");
    cursorText->setObjectName("cursorText");
    volumeSlider->setObjectName("volumeSlider");
    settingsButton->setObjectName("settingsButton");

    updateBackground();
    populateAlbums();

    connect(
        search,
        &QLineEdit::textChanged,
        this,
        [&](const QString& text)
        {
            searchText = text.trimmed();

            populateAlbums();
        }
    );

    connect(
        albums,
        &QListWidget::itemClicked,
        this,
        [&]
        {
            auto* item = albums->currentItem();

            selAlbum = item->data(Qt::UserRole).toInt();

            if (!audio._soundInit())
            {
                selTrack = -1;
            }

            populateTracks(selAlbum);
        }
    );

    connect(
        tracks,
        &QListWidget::itemClicked,
        this,
        [&]
        {
            selTrack = tracks->currentRow();
        }
    );

    connect(
        albums,
        &QListWidget::itemDoubleClicked,
        this,
        [&]
        {
            auto* item = albums->currentItem();

            playFirstOfAlbum(item->data(Qt::UserRole).toInt());
        }
    );

    connect(
        tracks,
        &QListWidget::itemDoubleClicked,
        this,
        &MainWindow::playSelected
    );

    connect(
        coverLabel,
        &ClickLabel::clicked,
        this,
        [this]
        {
            if (currentCover.isNull())
            {
                return;
            }

            showCoverFullscreen(
                currentCover,
                settings->coverNewWindow,
                nowPlaying->text(),
                this
            );
        }
    );

    connect(
        backwardButton,
        &QPushButton::clicked,
        this,
        [&]
        {
            play(
                curAlbum,
                curTrack - 1
            );

            if (viewedAlbumIndex() == curAlbum)
            {
                tracks->setCurrentRow(visibleRowForTrackIndex(curTrack));
            }
        }
    );

    connect(
        playPauseButton,
        &QPushButton::clicked,
        this,
        &MainWindow::playSelected
    );

    connect(
        forwardButton,
        &QPushButton::clicked,
        this,
        [&]
        {
            if (curAlbum < 0 || curTrack < 0)
            {
                return;
            }

            const auto& album = library.getAlbums()[curAlbum];

            if (!(curTrack + 1 >= int(album.tracks.size())))
            {
                play(
                    curAlbum,
                    curTrack + 1
                );

                if (viewedAlbumIndex() == curAlbum)
                {
                    tracks->setCurrentRow(visibleRowForTrackIndex(curTrack));
                }
            }
        }
    );

    connect(
        autoplay,
        &QCheckBox::toggled,
        this,
        [&](bool enabled)
        {
            settings->autoplay = enabled;
            settings->save();
        }
    );

    connect(
        cursorSlider,
        &QSlider::valueChanged,
        this,
        [&](int v)
        {
            if (audio._soundInit()
                && cursorSlider->isSliderDown())
            {
                audio.seek((v / double(cursorSlider->maximum())) * audio.length());
            }
        }
    );

    connect(
        cursorSlider,
        &QSlider::sliderMoved,
        this,
        [&](int v)
        {
            if (audio._soundInit())
            {
                audio.seek((v / double(cursorSlider->maximum())) * audio.length());
            }
        }
    );

    connect(
        volumeSlider,
        &QSlider::valueChanged,
        this,
        [&](int v)
        {
            settings->volume = v / 100.0f;
            settings->save();

            audio.setVolume(settings->volume);
        }
    );

    connect(
        settingsButton,
        &QPushButton::clicked,
        this,
        &MainWindow::openSettings
    );

    nam = new QNetworkAccessManager(this);

    connect(
        &timer,
        &QTimer::timeout,
        this,
        [&]
        {
            if (audio._soundInit())
            {
                const double len = audio.length();
                const double pos = audio.cursor();

                if (!scrobbledThisTrack
                    && len > 0.0)
                {
                    const double frac = pos / len;

                    if (frac >= scrobbleThreshold)
                    {
                        if (curAlbum >= 0 && curTrack >= 0)
                        {
                            lastfmScrobbleTrack(library.getAlbums()[curAlbum].tracks[curTrack]);
                        }

                        scrobbledThisTrack = true;
                    }
                }

                if (audio.finished())
                {
                    if (settings->autoplay)
                    {
                        const int viewedAlbum = viewedAlbumIndex();

                        if (viewedAlbum != curAlbum)
                        {
                            const auto& album = library.getAlbums()[curAlbum];

                            const int nt = curTrack + 1;

                            if (nt < int(album.tracks.size()))
                            {
                                play(curAlbum, nt);
                            }
                            else
                            {
                                audio.stop();

                                curAlbum = -1;
                                curTrack = -1;

                                updateNowPlaying();
                            }

                            return;
                        }

                        const int row = visibleRowForTrackIndex(curTrack);

                        if (row >= 0
                            && row + 1 < int(searchTrackOrder.size()))
                        {
                            const int nextTrackIndex = searchTrackOrder[row + 1];

                            play(curAlbum, nextTrackIndex);

                            tracks->setCurrentRow(row + 1);
                        }
                        else
                        {
                            audio.stop();

                            curAlbum = -1;
                            curTrack = -1;

                            //lastfmUpdateNowPlaying();
                            updateNowPlaying();
                        }
                    }

                    return;
                }

                if (audio.length() > 0)
                {
                    cursorText->setText(audio.formattedCursor() + " / " + audio.formattedLength());

                    if (!cursorSlider->isSliderDown())
                    {
                        cursorSlider->setValue(int(audio.cursor() / audio.length() * cursorSlider->maximum()));
                    }
                }
                else
                {
                    cursorText->setText("");
                }
            }
        }
    );

    auto playPauseShortcut = new QShortcut(
        QKeySequence(Qt::Key_Space),
        this
    );

    playPauseShortcut->setContext(Qt::ApplicationShortcut);

    connect(
        playPauseShortcut,
        &QShortcut::activated,
        this,
        &MainWindow::playSelected
    );

    initDriveWatcher();

    timer.start(10);
}

void MainWindow::updateControlsText()
{
    if (settings->iconButtons)
    {
        const QSize iconSize(14, 14);

        iconButtonsList[0]->setIcon(backwardIcon);
        iconButtonsList[1]->setIcon(playPauseIcon);
        iconButtonsList[2]->setIcon(forwardIcon);

        iconButtonsList[0]->setIconSize(iconSize);
        iconButtonsList[1]->setIconSize(iconSize);
        iconButtonsList[2]->setIconSize(iconSize);

        iconButtonsList[0]->setText("");
        iconButtonsList[1]->setText("");
        iconButtonsList[2]->setText("");
    }
    else
    {
        iconButtonsList[0]->setText("backward");
        iconButtonsList[1]->setText("play / pause");
        iconButtonsList[2]->setText("forward");

        iconButtonsList[0]->setIcon(QIcon());
        iconButtonsList[1]->setIcon(QIcon());
        iconButtonsList[2]->setIcon(QIcon());
    }
}

const QString MainWindow::customBackgroundStyleSheet = R"(
    #search,
    #albums,
    #tracks,
    #nowPlaying,
    #backwardButton,
    #playPauseButton,
    #forwardButton,
    #settingsButton
    {
        background-color: rgba(26, 26, 26, 128);
    }

    QListWidget::item:selected,
    QListWidget::item:selected:!active,
    QListWidget::item:hover,
    #backwardButton:hover,
    #playPauseButton:hover,
    #forwardButton:hover,
    #settingsButton:hover,
    #backwardButton:pressed,
    #playPauseButton:pressed,
    #forwardButton:pressed,
    #settingsButton:pressed
    {
        background-color: rgba(51, 51, 51, 128);
    }

    #nowPlaying
    {
        padding: 3px 5px;
    }

    #nowPlayingContainer,
    #autoplay,
    #cursorSlider,
    #cursorText,
    #volumeSlider
    {
        background: transparent;
    }
)";

void MainWindow::updateBackground()
{
    const QString p = settings->backgroundImagePath;

    if (p.isEmpty()
        || !QFileInfo::exists(p))
    {
        setAttribute(Qt::WA_NoSystemBackground, false);
        setAttribute(Qt::WA_OpaquePaintEvent, false);

        qApp->setStyleSheet(mainStyleSheet);

        updateAnimatedBackground(
            this,
            QString(),
            false
        );

        updateImageBackground(
            this,
            QString(),
            false
        );

        update();
        repaint();

        return;
    }

    const bool isGif = p.endsWith(".gif", Qt::CaseInsensitive);

    if (isGif)
    {
        setStyleSheet("");

        updateImageBackground(
            this,
            QString(),
            false
        );

        updateAnimatedBackground(
            this,
            p,
            settings->fillBackground
        );
    }
    else
    {
        updateAnimatedBackground(
            this,
            QString(),
            false
        );

        updateImageBackground(
            this,
            p,
            settings->fillBackground
        );
    }

    qApp->setStyleSheet(mainStyleSheet + customBackgroundStyleSheet);
}

static QString joinArtists(const std::vector<std::string>& artists)
{
    QStringList o;

    for (const auto& a : artists)
    {
        o << qs(a);
    }

    return o.join(", ");
}

void MainWindow::populateAlbums()
{
    selTrack = -1;
    selAlbum = -1;

    albums->clear();
    tracks->clear();

    albumMatchedByAlbum.clear();

    const QString q = searchText.trimmed().toLower();
    const auto& albumsVec = library.getAlbums();

    for (int i = 0; i < int(albumsVec.size()); ++i)
    {
        const auto& a = albumsVec[i];

        QString artistText = a.variousArtists
            ? "various artists"
            : qs(a.artists.front());

        bool artistMatch = q.isEmpty()
            || artistText.toLower().contains(q);

        bool albumMatch = q.isEmpty()
            || qs(a.title).toLower().contains(q);

        bool trackMatch = false;

        for (const auto& t : a.tracks)
        {
            const QString trackTitle = qs(t.title).toLower();
            const QString trackArtists = joinArtists(t.artists).toLower();

            if (trackTitle.contains(q)
                || trackArtists.contains(q))
            {
                trackMatch = true;

                break;
            }
        }

        if (artistMatch || albumMatch)
        {
            albumMatchedByAlbum.insert(i);
        }

        if (artistMatch || albumMatch || trackMatch)
        {
            auto* item = new QListWidgetItem(artistText + " - " + qs(a.title));
            item->setData(Qt::UserRole, i);

            albums->addItem(item);
        }
    }
}

void MainWindow::populateTracks(int albumIndex)
{
    tracks->clear();

    searchTrackOrder.clear();

    const QString q = searchText.trimmed().toLower();
    const auto& album = library.getAlbums()[albumIndex];

    for (int i = 0; i < int(album.tracks.size()); ++i)
    {
        const auto& t = album.tracks[i];

        QString artistText = album.variousArtists
            ? qs(t.artists.front())
            : QString();

        QString displayText = album.variousArtists
            ? artistText + " - " + qs(t.title)
            : qs(t.title);

        if (settings->trackNumbers)
        {
            const int digits = QString::number(int(album.tracks.size())).size();

            const QString num = QString("%1").arg(
                i + 1,
                digits,
                10,
                QChar('0')
            );

            displayText = num + " - " + displayText;
        }

        const QString matchText = (qs(t.title) + " " + joinArtists(t.artists)).toLower();

        if (!q.isEmpty()
            && !albumMatchedByAlbum.contains(albumIndex)
            && !matchText.contains(q))
        {
            continue;
        }

        searchTrackOrder.push_back(i);

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, i);

        tracks->addItem(item);
    }
}

void MainWindow::playFirstOfAlbum(int albumIndex)
{
    play(albumIndex, 0);

    tracks->setCurrentRow(0);
}

void MainWindow::play(int a, int t)
{
    const auto& albumsVec = library.getAlbums();

    if (a < 0 || a >= int(albumsVec.size()))
    {
        return;
    }

    const auto& album = albumsVec[a];

    if (t < 0 || t >= int(album.tracks.size()))
    {
        t = 0;
    }

    selAlbum = a;
    selTrack = t;
    curAlbum = a;
    curTrack = t;

    scrobbledThisTrack = false;

    audio.play(QString::fromUtf8(album.tracks[t].path.data(), int(album.tracks[t].path.size())));

    const QString cursorTextLength = audio.formattedLength() + " / " + audio.formattedLength();

    const int cursorTextWidth = cursorText->fontMetrics().horizontalAdvance(cursorTextLength);

    cursorText->setMinimumWidth(cursorTextWidth);

    updateNowPlaying();

    if (!settings->lastfmSessionKey.isEmpty())
    {
        lastfmUpdateNowPlaying(album.tracks[t]);
    }
}

void MainWindow::playSelected()
{
    int a = -1;
    int t = -1;

    if (auto* ai = albums->currentItem())
    {
        a = ai->data(Qt::UserRole).toInt();
    }

    if (auto* ti = tracks->currentItem())
    {
        t = ti->data(Qt::UserRole).toInt();
    }

    if (a >= 0 && t >= 0)
    {
        if (a != curAlbum || t != curTrack)
        {
            play(a, t);

            return;
        }
    }
    else if (a >= 0)
    {
        if (a != curAlbum || curTrack < 0)
        {
            playFirstOfAlbum(a);

            return;
        }
    }

    if (audio._soundInit())
    {
        audio.toggle();
    }
}

void MainWindow::openSettings()
{
    auto refreshUi =
        [&]()
        {
            const QString playingPath = currentPlayingPath();

            if (!playingPath.isEmpty())
            {
                rebindCurrentByPath(playingPath);
            }

            int a = curAlbum;

            if (a < 0)
            {
                a = selAlbum;
            }

            if (a >= 0)
            {
                for (int i = 0; i < albums->count(); ++i)
                {
                    if (albums->item(i)->data(Qt::UserRole).toInt() == a)
                    {
                        albums->setCurrentRow(i);

                        selAlbum = a;

                        break;
                    }
                }

                populateTracks(a);

                if (curTrack >= 0)
                {
                    const int row = visibleRowForTrackIndex(curTrack);

                    if (row >= 0)
                    {
                        tracks->setCurrentRow(row);

                        selTrack = row;
                    }
                }
            }

            updateNowPlaying();
        };

    SettingsDialog dlg(
        settings->folders,
        settings->autoplay,
        settings->coverSize,
        settings->trackFormat,
        settings->backgroundImagePath,
        settings->fillBackground,
        settings->iconButtons,
        settings->coverNewWindow,
        settings->trackNumbers,
        settings->lastfmUsername,
        settings->lastfmSessionKey,
        this
    );

    connect(
        &dlg,
        &SettingsDialog::rescanRequested,
        this,
        [&]
        {
            std::vector<std::filesystem::path> roots;

            for (const auto& f : settings->folders)
            {
                roots.emplace_back(std::filesystem::path(f.toUtf8().constData()));
            }

            const QString playingPath = currentPlayingPath();

            QApplication::setOverrideCursor(Qt::WaitCursor);

            library.scan(roots);

            QApplication::restoreOverrideCursor();

            populateAlbums();

            if (!rebindCurrentByPath(playingPath))
            {
                curAlbum = -1;
                curTrack = -1;
            }

            updateNowPlaying();
        }
    );

    connect(
        &dlg,
        &SettingsDialog::lastfmLoggedIn,
        this,
        [this](const QString& key, const QString& user)
        {
            settings->lastfmSessionKey = key;
            settings->lastfmUsername = user;
            settings->save();
        }
    );

    connect(
        &dlg,
        &SettingsDialog::lastfmLoggedOut,
        this,
        [this]
        {
            settings->lastfmSessionKey.clear();
            settings->lastfmUsername.clear();
            settings->save();
        }
    );

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    const bool foldersChanged = dlg.selectedFolders() != settings->folders;

    settings->folders = dlg.selectedFolders();
    settings->coverSize = dlg.selectedCoverSize();
    settings->trackFormat = dlg.selectedTrackFormat();
    settings->backgroundImagePath = dlg.selectedBackgroundImagePath();
    settings->fillBackground = dlg.selectedFillBackground();
    settings->iconButtons = dlg.selectedIconButtons();
    settings->coverNewWindow = dlg.selectedCoverNewWindow();
    settings->trackNumbers = dlg.selectedTrackNumbers();

    //if (settings->trackFormat.isEmpty())
    //{
    //    settings->trackFormat = { "cover", "artist", "track" };
    //}

    settings->lastfmUsername = dlg.getlastfmUsername();
    settings->lastfmSessionKey = dlg.getlastfmSessionKey();
    settings->save();

    updateBackground();
    updateControlsText();

    autoplay->setChecked(settings->autoplay);

    if (foldersChanged)
    {
        std::vector<std::filesystem::path> roots;

        for (const auto& f : settings->folders)
        {
            roots.emplace_back(std::filesystem::path(f.toUtf8().constData()));
        }

        const QString playingPath = currentPlayingPath();

        QApplication::setOverrideCursor(Qt::WaitCursor);

        library.scan(roots);

        QApplication::restoreOverrideCursor();

        populateAlbums();

        if (!rebindCurrentByPath(playingPath))
        {
            curAlbum = -1;
            curTrack = -1;
        }
    }

    refreshUi();

    coverLabel->setFixedHeight(settings->coverSize);
}

bool MainWindow::showCoverEnabled() const
{
    return settings->trackFormat.contains("cover");
}

static QPixmap loadEmbeddedCover(const QString& filePath)
{
#ifdef _WIN32
    const wchar_t* path = reinterpret_cast<const wchar_t*>(filePath.utf16());
#else
    const QByteArray path = filePath.toUtf8();
#endif

    if (filePath.endsWith(".mp3", Qt::CaseInsensitive))
    {
#ifdef _WIN32
        TagLib::MPEG::File f(path);
#else
        TagLib::MPEG::File f(path.constData());
#endif

        if (auto* tag = f.ID3v2Tag())
        {
            auto frames = tag->frameListMap()["APIC"];

            if (!frames.isEmpty())
            {
                auto* pic = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());

                return QPixmap::fromImage(
                    QImage::fromData(
                        reinterpret_cast<const uchar*>(pic->picture().data()),
                        pic->picture().size()
                    )
                );
            }
        }
    }

    if (filePath.endsWith(".flac", Qt::CaseInsensitive))
    {
#ifdef _WIN32
        TagLib::FLAC::File f(path);
#else
        TagLib::FLAC::File f(path.constData());
#endif

        auto pics = f.pictureList();

        if (!pics.isEmpty())
        {
            const auto* pic = pics.front();

            return QPixmap::fromImage(
                QImage::fromData(
                    reinterpret_cast<const uchar*>(pic->data().data()),
                    pic->data().size()
                )
            );
        }
    }

    if (filePath.endsWith(".ogg", Qt::CaseInsensitive))
    {
#ifdef _WIN32
        TagLib::Ogg::Vorbis::File f(path);
#else
        TagLib::Ogg::Vorbis::File f(path.constData());
#endif

        if (!f.isValid())
        {
            return {};
        }

        TagLib::Ogg::XiphComment* x = f.tag();

        if (!x)
        {
            return {};
        }

        const auto props = x->properties();

        auto getFirst =
            [&](const char* key) -> QByteArray
            {
                const TagLib::String k(
                    key,
                    TagLib::String::UTF8
                );

                if (!props.contains(k))
                {
                    return {};
                }

                const TagLib::StringList sl = props.value(k);

                if (sl.isEmpty())
                {
                    return {};
                }

                return QByteArray::fromStdString(sl.front().to8Bit(true));
            };

        const QByteArray b64 = getFirst("METADATA_BLOCK_PICTURE");

        if (!b64.isEmpty())
        {
            const QByteArray decoded = QByteArray::fromBase64(b64);

            if (!decoded.isEmpty())
            {
                TagLib::ByteVector bv(
                    decoded.constData(),
                    decoded.size()
                );

                TagLib::FLAC::Picture pic(bv);

                const TagLib::ByteVector& data = pic.data();

                if (!data.isEmpty())
                {
                    return QPixmap::fromImage(
                        QImage::fromData(
                            reinterpret_cast<const uchar*>(data.data()),
                            data.size()
                        )
                    );
                }
            }
        }

        const QByteArray b64f = getFirst("COVERART");

        if (!b64f.isEmpty())
        {
            const QByteArray decoded = QByteArray::fromBase64(b64f);

            if (!decoded.isEmpty())
            {
                return QPixmap::fromImage(
                    QImage::fromData(
                        reinterpret_cast<const uchar*>(decoded.constData()),
                        decoded.size()
                    )
                );
            }
        }
    }

    return {};
}

void MainWindow::updateNowPlaying()
{
    if (curAlbum < 0 || curTrack < 0)
    {
        nowPlaying->setText("nothing playing");

        coverLabel->hide();

        return;
    }

    const auto& track = library.getAlbums()[curAlbum].tracks[curTrack];
    const QString trackPath = QString::fromUtf8(track.path.data(), int(track.path.size()));

    nowPlaying->setText(formatTrack(track));

    if (!showCoverEnabled())
    {
        coverLabel->hide();

        return;
    }

    QPixmap cover = loadEmbeddedCover(trackPath);

    currentCover = cover;

    if (!cover.isNull())
    {
        const int h = settings->coverSize;

        const QPixmap scaled = cover.scaledToHeight(
            h,
            Qt::SmoothTransformation
        );

        coverLabel->setFixedWidth(scaled.width());
        coverLabel->setFixedHeight(h);

        coverLabel->setPixmap(scaled);

        coverLabel->show();
    }
    else {
        coverLabel->hide();
    }
}

QString MainWindow::formatTrack(const Track& t) const
{
    const auto& album = library.getAlbums()[curAlbum];

    QStringList parts;

    for (const auto& f : settings->trackFormat)
    {
        if (f == "artist")
        {
            if (album.variousArtists)
            {
                parts << qs(t.artists.front());
            }
            else
            {
                parts << qs(album.artists.front());
            }
        }
        else if (f == "album")
        {
            parts << qs(album.title);
        }
        else if (f == "track")
        {
            parts << qs(t.title);
        }
    }

    return parts.join(" - ");
}

QString MainWindow::currentPlayingPath() const
{
    if (curAlbum < 0 || curTrack < 0)
    {
        return {};
    }

    const auto& track = library.getAlbums()[curAlbum].tracks[curTrack];

    return QString::fromUtf8(
        track.path.data(),
        int(track.path.size())
    );
}

bool MainWindow::rebindCurrentByPath(const QString& path)
{
    if (path.isEmpty())
    {
        return false;
    }

    const auto& albums = library.getAlbums();

    for (int ai = 0; ai < int(albums.size()); ++ai)
    {
        const auto& album = albums[ai];

        for (int ti = 0; ti < int(album.tracks.size()); ++ti)
        {
            const auto& t = album.tracks[ti];
            const QString p = QString::fromUtf8(
                t.path.data(),
                int(t.path.size())
            );

            if (p == path)
            {
                curAlbum = ai;
                curTrack = ti;

                return true;
            }
        }
    }

    return false;
}

void MainWindow::initDriveWatcher()
{
    lastMountedRoots = getMountedRootSet();

    drivePollTimer.setInterval(1000);

    connect(
        &drivePollTimer,
        &QTimer::timeout,
        this,
        &MainWindow::checkMountedVolumes
    );

    drivePollTimer.start();

    rescanDebounceTimer.setInterval(1000);
    rescanDebounceTimer.setSingleShot(true);

    connect(
        &rescanDebounceTimer,
        &QTimer::timeout,
        this,
        [&]()
        {
            std::vector<std::filesystem::path> roots;

            for (const auto& f : settings->folders)
            {
                roots.emplace_back(std::filesystem::path(f.toUtf8().constData()));
            }

            const QString playingPath = currentPlayingPath();

            QApplication::setOverrideCursor(Qt::WaitCursor);

            library.scan(roots);

            QApplication::restoreOverrideCursor();

            populateAlbums();

            if (!rebindCurrentByPath(playingPath))
            {
                curAlbum = -1;
                curTrack = -1;
            }

            updateNowPlaying();
        }
    );
}

void MainWindow::checkMountedVolumes()
{
    const QSet<QString> now = getMountedRootSet();

    if (now == lastMountedRoots)
    {
        return;
    }

    QSet<QString> added = now;
    QSet<QString> removed = lastMountedRoots;

    added.subtract(lastMountedRoots);
    removed.subtract(now);

    lastMountedRoots = now;

    const QSet<QString> used = getLibraryMountRoots();

    bool affectsLibrary = false;

    for (const QString& a : added)
    {
        if (used.contains(a))
        {
            affectsLibrary = true;

            break;
        }
    }

    if (!affectsLibrary)
    {
        for (const QString& r : removed)
        {
            if (used.contains(r))
            {
                affectsLibrary = true;

                break;
            }
        }
    }

    if (affectsLibrary)
    {
        rescanDebounceTimer.start();
    }
}

QSet<QString> MainWindow::getLibraryMountRoots() const
{
    QSet<QString> used;

    for (const auto& f : settings->folders)
    {
        const QString r = mountRootForPath(f);

        if (!r.isEmpty())
        {
            used.insert(r);
        }
    }

    return used;
}