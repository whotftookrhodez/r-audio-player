#include <QString>

#include "audioplayer.h"

AudioPlayer::~AudioPlayer()
{
    stop();

    if (engineInit)
    {
        ma_engine_uninit(&engine);

        engineInit = false;
    }
}

bool AudioPlayer::init()
{
    if (engineInit)
    {
        return true;
    }

    if (ma_engine_init(
        nullptr,
        &engine
    ) != MA_SUCCESS)
    {
        return false;
    }

    engineInit = true;

    return true;
}

bool AudioPlayer::play(const QString& path)
{
    if (!engineInit
        || path.isEmpty())
    {
        return false;
    }

    stop();

#ifdef _WIN32
    if (ma_sound_init_from_file_w(
        &engine,
        reinterpret_cast<const wchar_t*>(path.utf16()),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        &sound) != MA_SUCCESS)
    {
        return false;
    }
#else
    const QByteArray utf8Path = path.toUtf8();

    if (ma_sound_init_from_file(
        &engine,
        utf8Path.constData(),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        &sound) != MA_SUCCESS)
    {
        return false;
    }
#endif

    ma_sound_start(&sound);

    soundInit = true;

    return true;
}

void AudioPlayer::stop()
{
    if (soundInit)
    {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);

        soundInit = false;
    }
}

void AudioPlayer::toggle()
{
    if (soundInit)
    {
        if (ma_sound_is_playing(&sound))
        {
            ma_sound_stop(&sound);
        }
        else
        {
            ma_sound_start(&sound);
        }
    }
}

void AudioPlayer::pause()
{
    if (soundInit)
    {
        ma_sound_stop(&sound);
    }
}

void AudioPlayer::run()
{
    if (soundInit)
    {
        ma_sound_start(&sound);
    }
}

void AudioPlayer::setVolume(float volume)
{
    if (engineInit)
    {
        ma_engine_set_volume(
            &engine,
            volume
        );
    }
}

void AudioPlayer::seek(double seconds)
{
    if (soundInit)
    {
        const ma_uint32 sampleRate = ma_engine_get_sample_rate(&engine);

        ma_sound_seek_to_pcm_frame(
            &sound,
            static_cast<ma_uint64>(seconds * sampleRate)
        );
    }
}

double AudioPlayer::length() const
{
    if (!soundInit)
    {
        return 0.0;
    }

    ma_uint64 frame = 0;

    ma_sound_get_length_in_pcm_frames(
        &sound,
        &frame
    );

    return static_cast<double>(frame) / ma_engine_get_sample_rate(&engine);
}

double AudioPlayer::cursor() const
{
    if (!soundInit)
    {
        return 0.0;
    }

    ma_uint64 frame = 0;

    ma_sound_get_cursor_in_pcm_frames(
        &sound,
        &frame
    );

    return static_cast<double>(frame) / ma_engine_get_sample_rate(&engine);
}

bool AudioPlayer::_soundInit() const
{
    return soundInit;
}

bool AudioPlayer::playing() const
{
    return soundInit
        && ma_sound_is_playing(&sound);
}

bool AudioPlayer::finished() const
{
    if (!soundInit)
    {
        return false;
    }

    ma_uint64 totalFrames = 0;
    ma_uint64 currentFrame = 0;

    ma_sound_get_length_in_pcm_frames(
        &sound,
        &totalFrames
    );

    ma_sound_get_cursor_in_pcm_frames(
        &sound,
        &currentFrame
    );

    return currentFrame >= totalFrames;
}

QString AudioPlayer::formattedCursor() const
{
    const int len = int(length() + 0.5);
    const int lh = len / 3600;
    const int lm = (len % 3600) / 60;
    const int cur = int(cursor() + 0.5);
    const int h = cur / 3600;
    const int m = (cur % 3600) / 60;
    const int s = cur % 60;

    if (lh > 0)
    {
        return QString("%1:%2:%3").arg(
            h,
            lh >= 10
            ? 2
            : 1,
            10,
            QChar('0')
        ).arg(
            m,
            2,
            10,
            QChar('0')
        ).arg(
            s,
            2,
            10,
            QChar('0')
        );
    }

    return QString("%1:%2").arg(
        m,
        lm >= 10
        ? 2
        : 1,
        10,
        QChar('0')
    ).arg(
        s,
        2,
        10,
        QChar('0')
    );
}

QString AudioPlayer::formattedLength() const
{
    const int len = int(length() + 0.5);
    const int h = len / 3600;
    const int m = (len % 3600) / 60;
    const int s = len % 60;

    if (h > 0)
    {
        return QString("%1:%2:%3").arg(h).arg(
            m,
            2,
            10,
            QChar('0')
        ).arg(
            s,
            2,
            10,
            QChar('0')
        );
    }

    return QString("%1:%2").arg(m).arg(
        s,
        2,
        10,
        QChar('0')
    );
}