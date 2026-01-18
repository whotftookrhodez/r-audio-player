#define MINIAUDIO_IMPLEMENTATION

#ifdef _WIN32
#define MA_WIN32_USE_WCHAR
#endif

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

    ma_sound_set_volume(
        &sound,
        ma_engine_get_volume(&engine)
    );

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

        if (soundInit)
        {
            ma_sound_set_volume(
                &sound,
                volume
            );
        }
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