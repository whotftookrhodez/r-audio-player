#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QTimer>
#include <QSlider>

#include <QDateTime> // for nam
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>

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

    QLineEdit* search = nullptr;
    QString searchText;
    QVector<int> searchTrackOrder;
    QSet<int> albumMatchedByAlbum;
    QListWidget* albums = nullptr;
    QString formatTrack(const Track& t) const;
    QListWidget* tracks = nullptr;
    QLabel* coverLabel = nullptr;
    QLabel* nowPlaying = nullptr;
    QVector<QPushButton*> iconButtonsList;
    QCheckBox* autoplay = nullptr;
    QTimer timer;
    QSlider* cursorSlider = nullptr;
    QSlider* volumeSlider = nullptr;
    QNetworkAccessManager* nam = nullptr;

    void lastfmUpdateNowPlaying(const Track& t);
    void lastfmScrobbleTrack(const Track& t);
    void updateControlsText();
    void populateAlbums();
    void populateTracks(int albumIndex);
    void playFirstOfAlbum(int albumIndex);
    void play(int albumIndex, int trackIndex);
    void playSelected();
    void openSettings();
    void updateNowPlaying();

    int selAlbum = -1;
    int selTrack = -1;
    int curAlbum = -1;
    int curTrack = -1;

    double scrobbleThreshold = 0.5;

    bool scrobbledThisTrack = false;
    bool playingFromSearch = false;
    bool showCoverEnabled() const;
};