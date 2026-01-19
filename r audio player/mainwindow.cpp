// includes are alphabetical in files > 1000 lines

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QPixmap>
#include <QPushButton>
#include <QShortcut>
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

#include "clickslider.h"
#include "folderdialog.h"
#include "mainwindow.h"
#include "settingsdialog.h"

static QString qs(const std::string& s)
{
    return QString::fromUtf8(s.data(), int(s.size()));
}

static QString qs(const std::wstring& w)
{
    return QString::fromStdWString(w);
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
            background-color: #1a1a1a;
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

    library.scan(roots);

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

    coverLabel = new QLabel(this);
    coverLabel->setFixedSize(settings->coverSize, settings->coverSize);
    coverLabel->setScaledContents(true);
    coverLabel->hide();

    nowPlaying = new QLabel("nothing playing", this);
    nowPlaying->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

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

    updateControlsText();

    autoplay = new QCheckBox("autoplay", this);
    autoplay->setChecked(settings->autoplay);
    autoplay->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    autoplay->setFixedWidth(autoplay->sizeHint().width());

    cursorSlider = new ClickSlider(Qt::Horizontal, this);
    cursorSlider->setRange(0, 2147483647);

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
    controls->addWidget(volumeSlider);
    controls->addWidget(settingsButton);

    auto root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->addWidget(search);
    root->addLayout(lists);
    root->addWidget(nowPlayingContainer);
    root->addLayout(controls);

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
        [&]
        {
            auto* item = tracks->currentItem();

            play(
                curAlbum,
                item->data(Qt::UserRole).toInt()
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

                if (audio.finished()) // questionable (repeats)
                {
                    if (settings->autoplay)
                    {
                        const int row = visibleRowForTrackIndex(curTrack);

                        if (row >= 0
                            && row + 1 < int(searchTrackOrder.size()))
                        {
                            const int nextTrackIndex = searchTrackOrder[row + 1];

                            play(curAlbum, nextTrackIndex);

                            tracks->setCurrentRow(row + 1);
                        }
                    }

                    return;
                }

                if (audio.length() > 0
                    && !cursorSlider->isSliderDown())
                {
                    cursorSlider->setValue(int(audio.cursor() / audio.length() * cursorSlider->maximum()));
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

    timer.start(10);
}

void MainWindow::updateControlsText()
{
    if (settings->iconButtons)
    {
        iconButtonsList[0]->setText("⏮");
        iconButtonsList[1]->setText("⏯");
        iconButtonsList[2]->setText("⏭");
    }
    else
    {
        iconButtonsList[0]->setText("backward");
        iconButtonsList[1]->setText("play / pause");
        iconButtonsList[2]->setText("forward");
    }
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

            if (trackTitle.contains(q) || trackArtists.contains(q))
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
    curTrack = -1;
    curAlbum = albumIndex;

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

    playingFromSearch = !searchText.isEmpty()
        && !albumMatchedByAlbum.contains(a);

    selAlbum = a;
    selTrack = t;
    curAlbum = a;
    curTrack = t;

    scrobbledThisTrack = false;

    audio.play(QString::fromUtf8(album.tracks[t].path.data(), int(album.tracks[t].path.size())));

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
    SettingsDialog dlg(
        settings->folders,
        settings->autoplay,
        settings->coverSize,
        settings->trackFormat,
        settings->iconButtons,
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

            library.scan(roots);

            populateAlbums();
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

    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    const bool foldersChanged = dlg.selectedFolders() != settings->folders;

    settings->folders = dlg.selectedFolders();
    settings->coverSize = dlg.selectedCoverSize();
    settings->trackFormat = dlg.selectedTrackFormat();
    settings->iconButtons = dlg.selectedIconButtons();

    if (settings->trackFormat.isEmpty())
    {
        settings->trackFormat = { "cover", "artist", "track" };
    }

    settings->lastfmUsername = dlg.getlastfmUsername();
    settings->lastfmSessionKey = dlg.getlastfmSessionKey();
    settings->save();

    updateControlsText();

    autoplay->setChecked(settings->autoplay);

    if (foldersChanged)
    {
        std::vector<std::filesystem::path> roots;

        for (const auto& f : settings->folders)
        {
            roots.emplace_back(std::filesystem::path(f.toUtf8().constData()));
        }

        library.scan(roots);

        populateAlbums();
    }

    coverLabel->setFixedSize(settings->coverSize, settings->coverSize);

    updateNowPlaying();
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

    coverLabel->setFixedSize(settings->coverSize, settings->coverSize); // not needed, but nice to have

    QPixmap cover;

    cover = loadEmbeddedCover(trackPath);

    if (!cover.isNull())
    {
        coverLabel->setPixmap(
            cover.scaled(
                coverLabel->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation
            )
        );

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