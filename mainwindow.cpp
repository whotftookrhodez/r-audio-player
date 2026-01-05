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

static QString qs(const std::wstring& w)
{
    return QString::fromStdWString(w);
}

bool MainWindow::showCoverEnabled() const
{
    return settings->trackFormat.contains("cover");
}

static QPixmap loadEmbeddedCover(const QString& filePath)
{
#ifdef Q_OS_WIN
    const wchar_t* path = reinterpret_cast<const wchar_t*>(filePath.utf16());
#else
    const QByteArray path = filePath.toUtf8();
#endif

    if (filePath.endsWith(".mp3", Qt::CaseInsensitive)) {
#ifdef Q_OS_WIN
        TagLib::MPEG::File f(path);
#else
        TagLib::MPEG::File f(path.constData());
#endif

        if (auto* tag = f.ID3v2Tag()) {
            auto frames = tag->frameListMap()["APIC"];

            if (!frames.isEmpty()) {
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

    if (filePath.endsWith(".flac", Qt::CaseInsensitive)) {
#ifdef Q_OS_WIN
        TagLib::FLAC::File f(path);
#else
        TagLib::FLAC::File f(path.constData());
#endif

        auto pics = f.pictureList();

        if (!pics.isEmpty()) {
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

MainWindow::MainWindow(Settings* s) : settings(s)
{
    qApp->setStyleSheet(R"(
        /* base widgets */

        QWidget {
            background-color: #1a1a1a;
            color: #e6e6e6;
        }

        QMainWindow {
            background-color: #1a1a1a;
        }

        QDialog {
            background-color: #1a1a1a;
        }

        /* text inputs */

        QLineEdit {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            padding: 6px;
            selection-background-color: #333333;
            selection-color: #e6e6e6;
        }

        QLineEdit:focus {
            outline: none;
            border: 1px solid #333333;
        }

        /* labels */

        QLabel {
            background-color: #1a1a1a;
            color: #e6e6e6;
        }

        /* buttons */

        QPushButton {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            padding: 6px 10px;
        }

        QPushButton:hover {
            background-color: #333333;
        }

        QPushButton:pressed {
            background-color: #333333;
        }

        QPushButton:focus {
            outline: none;
        }

        /* checkboxes */

        QCheckBox {
            spacing: 6px;
        }

        QCheckBox::indicator {
            width: 14px;
            height: 14px;
            border: 1px solid #333333;
            background-color: #1a1a1a;
        }

        QCheckBox::indicator:checked {
            background-color: #333333;
        }

        QCheckBox::indicator:focus {
            outline: none;
        }

        /* list widgets */

        QListWidget {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            outline: none;
        }

        QListWidget::item {
            padding: 6px;
        }

        QListWidget::item:selected {
            background-color: #333333;
            color: #e6e6e6;
        }

        QListWidget::item:selected:!active {
            background-color: #333333;
        }

        QListWidget::item:hover {
            background-color: #333333;
        }

        /* sliders (including clickslider) */

        QSlider {
            background-color: #1a1a1a;
        }

        QSlider::groove:horizontal {
            height: 6px;
            background-color: #333333;
            border: none;
        }

        QSlider::handle:horizontal {
            width: 12px;
            margin: -4px 0;
            background-color: #1a1a1a;
            border: 1px solid #333333;
        }

        QSlider::handle:horizontal:hover {
            background-color: #333333;
        }

        QSlider::sub-page:horizontal {
            background-color: #333333;
        }

        QSlider::add-page:horizontal {
            background-color: #1a1a1a;
        }

        /* scrollbars */

        QScrollBar:vertical,
        QScrollBar:horizontal {
            background-color: #1a1a1a;
            border: none;
        }

        QScrollBar::handle:vertical,
        QScrollBar::handle:horizontal {
            background-color: #333333;
            min-width: 20px;
            min-height: 20px;
        }

        QScrollBar::add-line,
        QScrollBar::sub-line {
            background: none;
            border: none;
        }

        QScrollBar::add-page,
        QScrollBar::sub-page {
            background: none;
        }

        /* dialogs & lists inside dialogs */

        QListView {
            background-color: #1a1a1a;
            border: 1px solid #333333;
            outline: none;
        }

        QListView::item:selected {
            background-color: #333333;
        }

        /* remove dotted / checkerboard focus visuals everywhere */

        *:focus {
            outline: none;
        }
    )");

    audio.init();
    audio.setVolume(settings->volume);

    if (settings
        ->folders
        .isEmpty())
    {
        FolderDialog dlg(
            settings->folders,
            this
        );

        if (dlg.exec()) {
            settings->folders = dlg.selectedFolders();
        }
    }

    std::vector<std::wstring> roots;

    for (const auto& f : settings->folders)
        roots.push_back(f.toStdWString());

    library.scan(roots);

    search = new QLineEdit(this);
    search->setPlaceholderText("search");
    search->setClearButtonEnabled(true);
    search->setMinimumHeight(32);

    auto lists = new QHBoxLayout;

    albums = new QListWidget(this);
    tracks = new QListWidget(this);

    lists->setSpacing(8);
    lists->addWidget(albums, 1);
    lists->addWidget(tracks, 2);

    coverLabel = new QLabel(this);
    coverLabel->setFixedSize(64, 64);
    coverLabel->setScaledContents(true);
    coverLabel->hide();

    nowPlaying = new QLabel("nothing playing", this);
    nowPlaying->setAlignment(Qt::AlignVCenter | Qt::AlignRight);

    auto nowPlayingLayout = new QHBoxLayout;
    nowPlayingLayout->setSpacing(8);
    nowPlayingLayout->setAlignment(Qt::AlignCenter);
    nowPlayingLayout->addWidget(coverLabel);
    nowPlayingLayout->addWidget(nowPlaying);

    auto nowPlayingContainer = new QWidget(this);
    nowPlayingContainer->setLayout(nowPlayingLayout);

    auto prevBtn = new QPushButton("reverse", this);
    auto playBtn = new QPushButton("play / pause", this);
    auto nextBtn = new QPushButton("forward", this);

    auto playPauseShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    playPauseShortcut->setContext(Qt::ApplicationShortcut);

    connect(playPauseShortcut, &QShortcut::activated, this,
        &MainWindow::playSelected);

    autoplay = new QCheckBox("autoplay", this);
    autoplay->setChecked(settings->autoplay);
    autoplay->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    autoplay->setFixedWidth(autoplay->sizeHint().width());

    progress = new ClickSlider(Qt::Horizontal, this);
    progress->setRange(0, 1000000); // lmao

    volume = new ClickSlider(Qt::Horizontal, this);
    volume->setRange(0, 100);
    volume->setValue(int(settings->volume * 100));

    auto settingsBtn = new QPushButton("settings", this);

    const int h = prevBtn->sizeHint().height();
    progress->setMinimumHeight(h);
    volume->setMinimumHeight(h);

    auto controls = new QHBoxLayout;
    controls->setSpacing(8);
    controls->addWidget(prevBtn);
    controls->addWidget(playBtn);
    controls->addWidget(nextBtn);
    controls->addWidget(autoplay, 0, Qt::AlignHCenter);
    controls->addWidget(progress, 1);
    controls->addWidget(volume);
    controls->addWidget(settingsBtn);

    auto root = new QVBoxLayout(this);
    root->setSpacing(8);
    root->addWidget(search);
    root->addLayout(lists);
    root->addWidget(nowPlayingContainer);
    root->addLayout(controls);

    populateAlbums();

    connect(albums, &QListWidget::itemClicked, this, [&]
        {
            auto* item = albums->currentItem();

            if (!item) return;

            selAlbum = item->data(Qt::UserRole).toInt();

            if (!audio.loaded()) {
                selTrack = -1;
            }

            populateTracks(selAlbum);
        }
    );

    connect(tracks, &QListWidget::itemClicked, this, [&]
        {
            if (!audio.loaded()) {
                selTrack = tracks->currentRow();
            }
        }
    );

    connect(albums, &QListWidget::itemDoubleClicked, this, [&]
        {
            auto* item = albums->currentItem();

            if (!item) return;

            playFirstOfAlbum(item->data(Qt::UserRole).toInt());
        }
    );

    connect(tracks, &QListWidget::itemDoubleClicked, this, [&]
        {
            auto* item = tracks->currentItem();

            if (!item) return;

            play(curAlbum, item->data(Qt::UserRole).toInt());
        }
    );

    connect(search, &QLineEdit::textChanged, this, [&](const QString& text)
        {
            searchText = text.trimmed();

            populateAlbums();
        }
    );

    connect(prevBtn, &QPushButton::clicked, this, [&]
        {
            play(curAlbum, curTrack - 1);
        }
    );

    connect(playBtn, &QPushButton::clicked, this,
        &MainWindow::playSelected);

    connect(nextBtn, &QPushButton::clicked, this, [&]
        {
            if (curAlbum < 0 || curTrack < 0)
                return;

            const auto& album = library.getAlbums()[curAlbum];

            if (!(curTrack + 1 >= int(album.tracks.size()))) {
                play(curAlbum, curTrack + 1);
            }
        }
    );

    connect(autoplay, &QCheckBox::toggled, this, [&](bool enabled)
        {
            settings->autoplay = enabled;
            settings->save();
        }
    );

    connect(progress, &QSlider::sliderMoved, this, [&](int v)
        {
            if (audio.loaded()) {
                audio.seek((v / double(progress->maximum())) * audio.length());
            }
        }
    );

    connect(volume, &QSlider::valueChanged, this, [&](int v)
        {
            settings->volume = v / 100.0f;
            audio.setVolume(settings->volume);
            settings->save();
        }
    );

    connect(settingsBtn, &QPushButton::clicked, this,
        &MainWindow::openSettings);

    connect(&timer, &QTimer::timeout, this, [&]
        {
            if (audio.loaded()) {
                if (audio.finished() && settings->autoplay) {
                    const auto& album = library.getAlbums()[curAlbum];

                    if (curTrack + 1 < int(album.tracks.size())) {
                        play(curAlbum, curTrack + 1);
                    }

                    return;
                }

                if (audio.length() > 0 && !progress->isSliderDown()) {
                    progress->setValue(int(audio.position() / audio.length() * progress->maximum()));
                }
            }
        }
    );

    timer.start(1);
}

void MainWindow::openSettings()
{
    SettingsDialog dlg(
        settings->folders,
        settings->autoplay,
        settings->trackFormat,
        this
    );

    connect(&dlg, &SettingsDialog::rescanRequested, this, [&]
        {
            std::vector<std::wstring> roots;

            for (const auto& f : settings->folders)
                roots.push_back(f.toStdWString());

            library.scan(roots);

            populateAlbums();
        }
    );

    if (dlg.exec() != QDialog::Accepted) return;

    const bool foldersChanged =
        dlg.selectedFolders() != settings->folders;

    settings->folders = dlg.selectedFolders();
    settings->trackFormat = dlg.selectedTrackFormat();

    if (settings->trackFormat.isEmpty())
        settings->trackFormat = { "cover", "artist", "track" };

    settings->save();

    autoplay->setChecked(settings->autoplay);

    if (foldersChanged) {
        std::vector<std::wstring> roots;

        for (const auto& f : settings->folders)
            roots.push_back(f.toStdWString());

        library.scan(roots);

        populateAlbums();
    }

    updateNowPlaying();
}

void MainWindow::populateAlbums()
{
    albums->clear();
    tracks->clear();

    selAlbum = -1;
    selTrack = -1;

    albumMatchedByAlbum.clear();

    const QString q = searchText.trimmed().toLower();
    const auto& albumsVec = library.getAlbums();

    for (int i = 0; i < int(albumsVec.size()); ++i) {
        const auto& a = albumsVec[i];

        QString artistText =
            a.variousArtists
            ? "various artists"
            : (!a.artists.empty() ? qs(a.artists.front()) : QString());

        bool artistMatch =
            q.isEmpty() ||
            artistText.toLower().contains(q);

        bool albumMatch =
            q.isEmpty() ||
            qs(a.title).toLower().contains(q);

        bool trackMatch = false;

        for (const auto& t : a.tracks) {
            if (qs(t.title).toLower().contains(q)) {
                trackMatch = true;
            }
        }

        if (artistMatch || albumMatch)
            albumMatchedByAlbum.insert(i);

        if (artistMatch || albumMatch || trackMatch) {
            auto* item = new QListWidgetItem(artistText + " - " + qs(a.title));
            item->setData(Qt::UserRole, i);
            albums->addItem(item);
        }
    }
}

void MainWindow::populateTracks(int albumIndex)
{
    tracks->clear();
    curAlbum = albumIndex;
    curTrack = -1;

    searchTrackOrder.clear();

    const QString q = searchText.trimmed().toLower();
    const auto& album = library.getAlbums()[albumIndex];

    for (int i = 0; i < int(album.tracks.size()); ++i) {
        const auto& t = album.tracks[i];

        QString artistText =
            album.variousArtists
            ? qs(t.artists.front())
            : QString();

        QString displayText =
            album.variousArtists
            ? artistText + " - " + qs(t.title)
            : qs(t.title);

        if (!q.isEmpty()
            && !albumMatchedByAlbum.contains(albumIndex)
            && !displayText.toLower().contains(q))
            continue;

        searchTrackOrder.push_back(i);

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, i);
        tracks->addItem(item);
    }
}

void MainWindow::playSelected()
{
    int a = -1;
    int t = -1;

    if (auto* ai = albums->currentItem())
        a = ai->data(Qt::UserRole).toInt();

    if (auto* ti = tracks->currentItem())
        t = ti->data(Qt::UserRole).toInt();

    if (a >= 0 && t >= 0) {
        if (a != curAlbum || t != curTrack) {
            play(a, t);

            return;
        }
    }
    else if (a >= 0) {
        if (a != curAlbum || curTrack < 0) {
            playFirstOfAlbum(a);

            return;
        }
    }

    if (audio.loaded()) {
        audio.toggle();
    }
}

void MainWindow::playFirstOfAlbum(int albumIndex)
{
    play(albumIndex, 0);
}

void MainWindow::play(int a, int t)
{
    const auto& albumsVec = library.getAlbums();

    if (a < 0 || a >= int(albumsVec.size()))
        return;

    const auto& album = albumsVec[a];

    if (t < 0 || t >= int(album.tracks.size()))
        t = 0;

    playingFromSearch = !searchText.isEmpty() && !albumMatchedByAlbum.contains(a);

    curAlbum = a;
    curTrack = t;
    selAlbum = a;
    selTrack = t;

    audio.play(QString::fromStdWString(album.tracks[t].path));

    updateNowPlaying();
}

void MainWindow::updateNowPlaying()
{
    if (curAlbum < 0 || curTrack < 0) {
        nowPlaying->setText("nothing playing");
        coverLabel->hide();

        return;
    }

    const auto& track = library.getAlbums()[curAlbum].tracks[curTrack];
    const QString trackPath = QString::fromStdWString(track.path);

    nowPlaying->setText(formatTrack(track));

    if (!showCoverEnabled()) {
        coverLabel->hide();

        return;
    }

    QPixmap cover;

    cover = loadEmbeddedCover(trackPath);

    if (!cover.isNull()) {
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

    for (const auto& f : settings->trackFormat) {
        if (f == "artist") {
            if (album.variousArtists) {
                parts << qs(t.artists.front());
            }
            else {
                parts << qs(album.artists.front());
            }
        }
        else if (f == "album") {
            parts << qs(album.title);
        }
        else if (f == "track") {
            parts << qs(t.title);
        }
    }

    return parts.join(" - ");
}