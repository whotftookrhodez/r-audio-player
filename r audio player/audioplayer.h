#pragma once

#include <QString>

#include "miniaudio.h"

class AudioPlayer
{
public:
    AudioPlayer();
    ~AudioPlayer();

    bool init();
    bool play(const QString& path);

    void stop();
    void toggle();
    void pause();
    void resume();
    void setVolume(float volume);
    void seek(double seconds);

    double length() const;
    double position() const;

    bool loaded() const;
    bool playing() const;
    bool finished() const;
private:
    ma_engine engine{};
    ma_sound sound{};

    bool engineReady{ false };
    bool soundLoaded{ false };
};