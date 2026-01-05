#define MINIAUDIO_IMPLEMENTATION
#define MA_WIN32_USE_WCHAR

#include "audioplayer.h"

AudioPlayer::AudioPlayer()
    : engineReady(false),
    soundLoaded(false)
{
}

AudioPlayer::~AudioPlayer()
{
    stop();

    if (engineReady) {
        ma_engine_uninit(&engine);
        engineReady = false;
    }
}

bool AudioPlayer::init()
{
    if (engineReady) {
        return true;
    }

    if (ma_engine_init(
        nullptr,
        &engine) != MA_SUCCESS) {
        return false;
    }

    engineReady = true;

    return true;
}

bool AudioPlayer::play(const QString& path)
{
    if (!engineReady
        || path.isEmpty()) {
        return false;
    }

    stop();

    if (ma_sound_init_from_file_w(
        &engine,
        reinterpret_cast<const wchar_t*>(path.utf16()),
        MA_SOUND_FLAG_STREAM,
        nullptr,
        nullptr,
        &sound) != MA_SUCCESS) {
        return false;
    }

    ma_sound_set_volume(
        &sound,
        ma_engine_get_volume(&engine));
    ma_sound_start(&sound);
    soundLoaded = true;

    return true;
}

void AudioPlayer::stop()
{
    if (soundLoaded) {
        ma_sound_stop(&sound);
        ma_sound_uninit(&sound);
        soundLoaded = false;
    }
}

void AudioPlayer::toggle()
{
    if (soundLoaded) {
        if (ma_sound_is_playing(&sound)) {
            ma_sound_stop(&sound);
        }
        else {
            ma_sound_start(&sound);
        }
    }
}

void AudioPlayer::pause()
{
    if (soundLoaded) {
        ma_sound_stop(&sound);
    }
}

void AudioPlayer::resume()
{
    if (soundLoaded) {
        ma_sound_start(&sound);
    }
}

void AudioPlayer::setVolume(float volume)
{
    if (engineReady) {
        ma_engine_set_volume(
            &engine,
            volume);

        if (soundLoaded) {
            ma_sound_set_volume(
                &sound,
                volume);
        }
    }
}

void AudioPlayer::seek(double seconds)
{
    if (soundLoaded
        && seconds >= 0.0) {
        const ma_uint32 SAMPLE_RATE = ma_engine_get_sample_rate(&engine);

        ma_sound_seek_to_pcm_frame(
            &sound,
            static_cast<ma_uint64>(seconds * SAMPLE_RATE)
        );
    }
}

double AudioPlayer::length() const
{
    if (!soundLoaded) {
        return 0.0;
    }

    ma_uint64 frame = 0;

    ma_sound_get_length_in_pcm_frames(
        &sound,
        &frame);

    return frame / static_cast<double>(
        ma_engine_get_sample_rate(&engine));
}

double AudioPlayer::position() const
{
    if (!soundLoaded) {
        return 0.0;
    }

    ma_uint64 frame = 0;

    ma_sound_get_cursor_in_pcm_frames(
        &sound,
        &frame);

    return frame / static_cast<double>(
        ma_engine_get_sample_rate(&engine));
}

bool AudioPlayer::loaded() const
{
    return soundLoaded;
}

bool AudioPlayer::playing() const
{
    return soundLoaded
        && ma_sound_is_playing(&sound);
}

bool AudioPlayer::finished() const
{
    if (!soundLoaded) {
        return false;
    }

    ma_uint64 totalFrames = 0;
    ma_uint64 currentFrame = 0;

    ma_sound_get_length_in_pcm_frames(
        &sound,
        &totalFrames);
    ma_sound_get_cursor_in_pcm_frames(
        &sound,
        &currentFrame);

    return currentFrame >= totalFrames;
}