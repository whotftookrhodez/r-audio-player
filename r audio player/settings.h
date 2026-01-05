#pragma once

#include <QStringList>
#include <QSettings>

struct Settings {
    QStringList folders;

    bool autoplay = true;
    float volume = 1.0f;
    QStringList trackFormat = { "cover", "artist", "track" };

    static constexpr const char* K_FOLDERS = "folders";
    static constexpr const char* K_AUTOPLAY = "autoplay";
    static constexpr const char* K_VOLUME = "volume";
    static constexpr const char* K_TRACKFORMAT = "trackFormat";

    void load() {
        QSettings s;

        folders = s.value(K_FOLDERS, folders).toStringList();
        autoplay = s.value(K_AUTOPLAY, autoplay).toBool();
        volume = s.value(K_VOLUME, volume).toFloat();
        trackFormat = s.value(K_TRACKFORMAT, trackFormat).toStringList();
    }

    void save() const {
        QSettings s;

        s.setValue(K_FOLDERS, folders);
        s.setValue(K_AUTOPLAY, autoplay);
        s.setValue(K_VOLUME, volume);
        s.setValue(K_TRACKFORMAT, trackFormat);
    }
};