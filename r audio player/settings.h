#pragma once

#include <QStringList>
#include <QSettings>

struct Settings
{
    QStringList folders;
    bool autoplay = true;
    float volume = 1.0f;
    int coverSize = 70;
    QStringList trackFormat = { "cover", "artist", "track" };
    QString backgroundImagePath;
    bool fillBackground = true;
    bool iconButtons = true;
    bool coverNewWindow = true;
    bool trackNumbers = true;
    QString lastfmUsername;
    QString lastfmSessionKey;

    static constexpr const char* K_FOLDERS = "folders";
    static constexpr const char* K_AUTOPLAY = "autoplay";
    static constexpr const char* K_VOLUME = "volume";
    static constexpr const char* K_COVERSIZE = "coverSize";
    static constexpr const char* K_TRACKFORMAT = "trackFormat";
    static constexpr const char* K_BACKGROUNDIMAGEPATH = "backgroundImagePath";
    static constexpr const char* K_FILLBACKGROUND = "fillBackground";
    static constexpr const char* K_ICONBUTTONS = "iconButtons";
    static constexpr const char* K_COVERNEWWINDOW = "coverNewWindow";
    static constexpr const char* K_TRACKNUMBERS = "trackNumbers";
    static constexpr const char* K_LASTFM_USERNAME = "lastfmUsername";
    static constexpr const char* K_LASTFM_SESSIONKEY = "lastfmSessionKey";

    // why not
    static constexpr const char* LASTFM_API_KEY = "73141291e72e8cd841e0fbe8937b5b85";
    static constexpr const char* LASTFM_API_SECRET = "4859b947234fcddff386febc1f73b579";

    void load()
    {
        QSettings s;

        folders = s.value(K_FOLDERS, folders).toStringList();
        autoplay = s.value(K_AUTOPLAY, autoplay).toBool();
        volume = s.value(K_VOLUME, volume).toFloat();
        coverSize = s.value(K_COVERSIZE, coverSize).toInt();
        trackFormat = s.value(K_TRACKFORMAT, trackFormat).toStringList();
        backgroundImagePath = s.value(K_BACKGROUNDIMAGEPATH, backgroundImagePath).toString();
        fillBackground = s.value(K_FILLBACKGROUND, fillBackground).toBool();
        iconButtons = s.value(K_ICONBUTTONS, iconButtons).toBool();
        coverNewWindow = s.value(K_COVERNEWWINDOW, coverNewWindow).toBool();
        trackNumbers = s.value(K_TRACKNUMBERS, trackNumbers).toBool();
        lastfmUsername = s.value(K_LASTFM_USERNAME, "").toString();
        lastfmSessionKey = s.value(K_LASTFM_SESSIONKEY, "").toString();
    }

    void save() const
    {
        QSettings s;

        s.setValue(K_FOLDERS, folders);
        s.setValue(K_AUTOPLAY, autoplay);
        s.setValue(K_VOLUME, volume);
        s.setValue(K_COVERSIZE, coverSize);
        s.setValue(K_TRACKFORMAT, trackFormat);
        s.setValue(K_BACKGROUNDIMAGEPATH, backgroundImagePath);
        s.setValue(K_FILLBACKGROUND, fillBackground);
        s.setValue(K_ICONBUTTONS, iconButtons);
        s.setValue(K_COVERNEWWINDOW, coverNewWindow);
        s.setValue(K_TRACKNUMBERS, trackNumbers);
        s.setValue(K_LASTFM_USERNAME, lastfmUsername);
        s.setValue(K_LASTFM_SESSIONKEY, lastfmSessionKey);
    }
};