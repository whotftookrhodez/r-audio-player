#pragma once

#include <QStringList>
#include <QSettings>

struct Settings
{
    QStringList folders;
    bool autoplay = true;
    float volume = 1.0f;
    QStringList trackFormat = { "cover", "artist", "track" };
    bool iconButtons = false;
    QString lastfmUsername;
    QString lastfmSessionKey;

    static constexpr const char* K_FOLDERS = "folders";
    static constexpr const char* K_AUTOPLAY = "autoplay";
    static constexpr const char* K_VOLUME = "volume";
    static constexpr const char* K_TRACKFORMAT = "trackFormat";
    static constexpr const char* K_ICONBUTTONS = "iconButtons";
    static constexpr const char* K_LASTFM_USERNAME = "lastfmUsername";
    static constexpr const char* K_LASTFM_SESSIONKEY = "lastfmSessionKey";

    // hide in release
    static constexpr const char* LASTFM_API_KEY = "";
    static constexpr const char* LASTFM_API_SECRET = "";

    void load()
    {
        QSettings s;

        folders = s.value(K_FOLDERS, folders).toStringList();
        autoplay = s.value(K_AUTOPLAY, autoplay).toBool();
        volume = s.value(K_VOLUME, volume).toFloat();
        trackFormat = s.value(K_TRACKFORMAT, trackFormat).toStringList();
        iconButtons = s.value(K_ICONBUTTONS, iconButtons).toBool();
        lastfmUsername = s.value(K_LASTFM_USERNAME, "").toString();
        lastfmSessionKey = s.value(K_LASTFM_SESSIONKEY, "").toString();
    }

    void save() const
    {
        QSettings s;

        s.setValue(K_FOLDERS, folders);
        s.setValue(K_AUTOPLAY, autoplay);
        s.setValue(K_VOLUME, volume);
        s.setValue(K_TRACKFORMAT, trackFormat);
        s.setValue(K_ICONBUTTONS, iconButtons);
        s.setValue(K_LASTFM_USERNAME, lastfmUsername);
        s.setValue(K_LASTFM_SESSIONKEY, lastfmSessionKey);
    }
};