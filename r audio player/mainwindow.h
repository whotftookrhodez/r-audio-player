#pragma once

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSlider>
#include <QTimer>
#include <QWidget>

#include "audioplayer.h"
#include "library.h"
#include "settings.h"

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(Settings* settings);
private:
    QCheckBox* autoplay = nullptr;
    QLabel* coverLabel;
    QLabel* nowPlaying = nullptr;
    QLineEdit* search;
    QListWidget* albums = nullptr;
    QListWidget* tracks = nullptr;
    QSet<int> albumMatchedByAlbum;
    QSlider* progress = nullptr;
    QSlider* volume = nullptr;
    QString formatTrack(const Track& t) const;
    QString searchText;
    QTimer timer;
    QVector<int> searchTrackOrder;
    QString lastAlbumTitle;
    QString lastAlbumArtist;

    Settings* settings = nullptr;
    Library library;
    AudioPlayer audio;

    int curAlbum = -1;
    int curTrack = -1;
    int selAlbum = -1;
    int selTrack = -1;

    bool playingFromSearch = false;
    bool showCoverEnabled() const;

    void openSettings();
    void populateAlbums();
    void populateTracks(int albumIndex);
    void playSelected();
    void playFirstOfAlbum(int albumIndex);
    void play(int albumIndex, int trackIndex);
    void updateNowPlaying();
};