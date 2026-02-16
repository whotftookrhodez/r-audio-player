#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QSet>
#include <QSlider>
#include <QString>

#include <QDateTime> // for nam
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "clicklabel.h"
#include "settings.h"
#include "library.h"
#include "audioplayer.h"

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(Settings* settings);
private:
    Settings* settings = nullptr;
    Library library;
    AudioPlayer audio;

    QString mainStyleSheet;
    QLineEdit* search = nullptr;
    QString searchText;
    QVector<int> searchTrackOrder;
    QSet<int> albumMatchedByAlbum;
    QListWidget* albums = nullptr;
    QString formatTrack(const Track& t) const;
    QListWidget* tracks = nullptr;
    QPixmap currentCover;
    ClickLabel* coverLabel = nullptr;
    QLabel* nowPlaying = nullptr;
    QVector<QPushButton*> iconButtonsList;
    QIcon backwardIcon;
    QIcon playPauseIcon;
    QIcon forwardIcon;
    QCheckBox* autoplay = nullptr;
    QTimer timer;
    QSlider* cursorSlider = nullptr;
    QLabel* cursorText = nullptr;
    QSlider* volumeSlider = nullptr;
    QNetworkAccessManager* nam = nullptr;
    QString currentPlayingPath() const;
    QTimer drivePollTimer;
    QTimer rescanDebounceTimer;
    QSet<QString> lastMountedRoots;
    QSet<QString> getLibraryMountRoots() const;

    static const QString customBackgroundStyleSheet;

    int viewedAlbumIndex() const;
    int visibleRowForTrackIndex(int trackIndex) const;
    int selAlbum = -1;
    int selTrack = -1;
    int curAlbum = -1;
    int curTrack = -1;

    void lastfmUpdateNowPlaying(const Track& t);
    void lastfmScrobbleTrack(const Track& t);
    void updateControlsText();
    void updateBackground();
    void populateAlbums();
    void populateTracks(int albumIndex);
    void playFirstOfAlbum(int albumIndex);
    void play(int albumIndex, int trackIndex);
    void playSelected();
    void openSettings();
    void updateNowPlaying();
    void initDriveWatcher();
    void checkMountedVolumes();

    double scrobbleThreshold = 0.9;

    bool scrobbledThisTrack = false;
    bool showCoverEnabled() const;
    bool rebindCurrentByPath(const QString& path);
};