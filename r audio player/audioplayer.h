#pragma once

#include <QString>

#include "miniaudio.h"

class AudioPlayer
{
public:
    ~AudioPlayer();

    bool init();
    bool play(const QString& path);

    void stop();
    void toggle();
    void pause();
    void run();
    void setVolume(float volume);
    void seek(double seconds);

    double length() const;
    double cursor() const;

    bool _soundInit() const;
    bool playing() const;
    bool finished() const;
private:
    bool engineInit{};
    bool soundInit{};

    ma_engine engine{};
    ma_sound sound{};
};