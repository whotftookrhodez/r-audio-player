#include <filesystem>
#include <fstream>
#include <string>
#include <algorithm>
#include <set>
#include <system_error>

#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/tag.h>
#include <taglib/wavfile.h>
#include <taglib/oggfile.h>
#include <taglib/vorbisfile.h>

#include "library.h"

namespace fs = std::filesystem; // YOU DESERVE DEATH FOR THIS - some senior dev, probably

static std::string u8ToString(const std::filesystem::path& p)
{
    auto u8 = p.u8string();

    return std::string(u8.begin(), u8.end());
}

static bool isVorbis(const std::filesystem::path& p)
{
#ifdef _WIN32
    const std::wstring wp = p.wstring();

    std::ifstream f(
        wp,
        std::ios::binary
    );
#else
    std::ifstream f(
        p,
        std::ios::binary
    );
#endif
    if (!f)
    {
        return false;
    }

    char buf[64]{};

    f.read(
        buf,
        sizeof(buf)
    );

    const std::streamsize n = f.gcount();

    if (n < 40)
    {
        return false;
    }

    if (!(buf[0] == 'O'
        && buf[1] == 'g'
        && buf[2] == 'g'
        && buf[3] == 'S'))
    {
        return false;
    }

    for (int i = 0; i + 6 < (int)n; ++i)
    {
        if ((unsigned char)buf[i] == 0x01
            && buf[i + 1] == 'v'
            && buf[i + 2] == 'o'
            && buf[i + 3] == 'r'
            && buf[i + 4] == 'b'
            && buf[i + 5] == 'i'
            && buf[i + 6] == 's')
        {
            return true;
        }
    }

    return false;
}

namespace
{
    std::string toLowerAscii(std::string s)
    {
        for (char& c : s)
        {
            if (c >= 'A'
                && c <= 'Z')
            {
                c = static_cast<char>(c - 'A' + 'a');
            }
        }

        return s;
    }

    inline bool isAsciiSpace(char c)
    {
        return c == ' '
            || c == '\f'
            || c == '\n'
            || c == '\r'
            || c == '\t'
            || c == '\v';
    }

    bool isAudioFile(const fs::path& p)
    {
        if (!p.has_extension())
        {
            return false;
        }

        std::string ext = u8ToString(p.extension());
        ext = toLowerAscii(ext);

        return ext == ".wav"
            || ext == ".mp3"
            || ext == ".flac"
            || ext == ".ogg";
    }

    TagLib::FileRef openFileRef(const fs::path& p)
    {
#ifdef _WIN32
        const std::wstring wp = p.wstring();

        return TagLib::FileRef(wp.c_str());
#else
        const std::string up = u8ToString(p);

        return TagLib::FileRef(up.c_str());
#endif
    }

    std::string trimAscii(std::string s)
    {
        auto b = s.begin();
        auto e = s.end();

        while (b != e
            && isAsciiSpace(*b))
        {
            ++b;
        }

        while (e != b
            && isAsciiSpace(*(e - 1)))
        {
            --e;
        }

        s.erase(s.begin(), b);
        s.erase(e, s.end());

        return s;
    }

    std::string toUtf8(const TagLib::String& s)
    {
        return s.to8Bit(true);
    }

    std::string extractArtistLiteral(TagLib::FileRef& ref)
    {
        if (ref.tag())
        {
            std::string artist = trimAscii(toUtf8(ref.tag()->artist()));

            if (!artist.empty())
            {
                return artist;
            }
        }

        if (ref.file())
        {
            const TagLib::PropertyMap props = ref.file()->properties();

            for (auto it = props.begin(); it != props.end(); ++it)
            {
                const std::string key = toLowerAscii(toUtf8(it->first));

                if (key.find("artist") == std::string::npos)
                {
                    continue;
                }

                if (!it->second.isEmpty())
                {
                    return trimAscii(toUtf8(it->second.front()));
                }
            }
        }

        return {};
    }

    std::string pathToUtf8String(const fs::path& p)
    {
        return u8ToString(p);
    }
}

void Library::scan(const std::vector<fs::path>& roots)
{
    albums.clear();
    albumIndex.clear();

    for (const auto& root : roots)
    {
        scanFolderRecursive(root);
    }

    for (auto& album : albums)
    {
        std::set<std::string> artistSet;

        for (const auto& track : album.tracks)
        {
            artistSet.insert(track.artists.front());
        }

        if (artistSet.size() == 1)
        {
            album.artists = { *artistSet.begin() };
            album.variousArtists = false;
        }
        else
        {
            album.artists.clear();
            album.variousArtists = true;
        }

        std::sort(
            album.tracks.begin(),
            album.tracks.end(),
            [](const Track& a, const Track& b)
            {
                return a.trackNo < b.trackNo;
            }
        );
    }

    std::sort(
        albums.begin(),
        albums.end(),
        [](const Album& a, const Album& b)
        {
            const auto key = [](const Album& x)
                {
                    const std::string artist = x.variousArtists
                        ? "various artists"
                        : x.artists.front();

                    return toLowerAscii(artist + " - " + x.title);
                };

            return key(a) < key(b);
        }
    );
}

void Library::scanFolderRecursive(const fs::path& folder)
{
    std::error_code ec;

    if (!fs::exists(folder, ec)
        || !fs::is_directory(folder, ec))
    {
        return;
    }

    fs::recursive_directory_iterator it(
        folder,
        fs::directory_options::skip_permission_denied,
        ec
    );

    for (; it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec
            || !it->is_regular_file(ec))
        {
            continue;
        }

        fs::path path = it->path();

        if (!isAudioFile(path))
        {
            continue;
        }

        const std::string ext = toLowerAscii(u8ToString(path.extension()));

        if (ext == ".ogg"
            && !isVorbis(path))
        {
            continue;
        }

        Track track{};

        track.path = pathToUtf8String(path);

        if (ext == ".wav")
        {
#ifdef _WIN32
            TagLib::RIFF::WAV::File file(path.wstring().c_str());
#else
            TagLib::RIFF::WAV::File file(u8ToString(path).c_str());
#endif
            if (!file.isValid())
            {
                continue;
            }

            std::string artist;

            if (file.tag())
            {
                artist = trimAscii(toUtf8(file.tag()->artist()));

                track.title = trimAscii(toUtf8(file.tag()->title()));
                track.trackNo = file.tag()->track();
                track.album = trimAscii(toUtf8(file.tag()->album()));
            }

            if (artist.empty()
                && !track.title.empty())
            {
                const TagLib::PropertyMap props = file.properties();

                for (auto itp = props.begin(); itp != props.end(); ++itp)
                {
                    const std::string key = toLowerAscii(toUtf8(itp->first));

                    if (key.find("artist") != std::string::npos
                        && !itp->second.isEmpty())
                    {
                        artist = trimAscii(toUtf8(itp->second.front()));

                        break;
                    }
                }
            }

            if (artist.empty())
            {
                continue;
            }

            track.artists = { artist };
        }
        else if (ext == ".ogg")
        {
#ifdef _WIN32
            TagLib::Ogg::Vorbis::File file(path.wstring().c_str());
#else
            TagLib::Ogg::Vorbis::File file(u8ToString(path).c_str());
#endif
            if (!file.isValid())
            {
                continue;
            }

            std::string artist;

            if (file.tag())
            {
                artist = trimAscii(toUtf8(file.tag()->artist()));

                track.title = trimAscii(toUtf8(file.tag()->title()));
                track.trackNo = file.tag()->track();
                track.album = trimAscii(toUtf8(file.tag()->album()));
            }

            if (artist.empty()
                && !track.title.empty())
            {
                const TagLib::PropertyMap props = file.properties();

                for (auto itp = props.begin(); itp != props.end(); ++itp)
                {
                    const std::string key = toLowerAscii(toUtf8(itp->first));

                    if (key.find("artist") != std::string::npos
                        && !itp->second.isEmpty())
                    {
                        artist = trimAscii(toUtf8(itp->second.front()));

                        break;
                    }
                }
            }

            if (artist.empty())
            {
                continue;
            }

            track.artists = { artist };
        }
        else
        {
            TagLib::FileRef ref = openFileRef(path);

            if (ref.isNull())
            {
                continue;
            }

            const std::string artist = extractArtistLiteral(ref);

            if (artist.empty())
            {
                continue;
            }

            track.artists = { artist };

            if (ref.tag())
            {
                auto* tag = ref.tag();

                track.title = trimAscii(toUtf8(tag->title()));
                track.trackNo = tag->track();
                track.album = trimAscii(toUtf8(tag->album()));
            }
        }

        if (track.title.empty())
        {
            continue;
        }

        if (track.trackNo == 0)
        {
            if (track.album.empty()
                || track.album == track.title)
            {
                track.trackNo = 1;
            }
            else
            {
                continue;
            }
        }

        if (track.album.empty())
        {
            track.album = track.title;
        }

        addTrack(std::move(track));
    }
}

void Library::addTrack(Track&& track)
{
    const std::string key = toLowerAscii(track.album);
    const auto it = albumIndex.find(key);

    if (it == albumIndex.end())
    {
        Album album;

        album.title = track.album;
        album.tracks.push_back(std::move(track));

        const size_t index = albums.size();

        albums.push_back(std::move(album));
        albumIndex.emplace(key, index);
    }
    else
    {
        albums[it->second].tracks.push_back(std::move(track));
    }
}