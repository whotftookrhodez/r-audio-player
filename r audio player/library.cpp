#include "library.h"

#include <cwctype>
#include <algorithm>
#include <set>
#include <filesystem>
#include <system_error>

#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/fileref.h>

namespace fs = std::filesystem;

namespace
{
    std::wstring trim(std::wstring s)
    {
        const auto notSpace = [](wchar_t c)
            { return !std::iswspace(c); };

        s.erase(
            s.begin(),
            std::find_if(
                s.begin(),
                s.end(),
                notSpace
            )
        );

        s.erase(
            std::find_if(
                s.rbegin(),
                s.rend(),
                notSpace
            ).base(),
            s.end()
        );

        return s;
    }

    std::wstring toLower(std::wstring s)
    {
        for (auto& c : s) {
            c = std::towlower(c);
        }

        return s;
    }

    std::wstring extractArtistLiteral(TagLib::FileRef& ref)
    {
        if (ref.tag()) {
            const std::wstring artist = trim(
                ref
                .tag()
                ->artist()
                .toWString()
            );

            if (!artist.empty()) {
                return artist;
            }
        }

        if (ref.file()) {
            const TagLib::PropertyMap props =
                ref
                .file()
                ->properties();

            for (auto it = props.begin(); it != props.end(); ++it) {
                const std::wstring key =
                    toLower(
                        it
                        ->first
                        .toWString()
                    );

                if (key.find(L"artist") == std::wstring::npos) {
                    continue;
                }

                if (!it
                    ->second
                    .isEmpty())
                {
                    return trim(it->second.front().toWString());
                }
            }
        }

        return {};
    }

}

bool Library::isAudioFile(const std::wstring& ext)
{
    const std::wstring e = toLower(ext);

    return e == L".wav"
        || e == L".mp3"
        || e == L".flac";
}

void Library::scan(const std::vector<std::wstring>& roots)
{
    albums.clear();
    albumIndex.clear();

    for (const auto& root : roots) {
        scanFolderRecursive(root);
    }

    for (auto& album : albums) {
        std::set<std::wstring> artistSet;

        for (const auto& track : album.tracks) {
            artistSet.insert(track.artists.front());
        }

        if (artistSet.size() == 1) {
            album.artists = { *artistSet.begin() };
            album.variousArtists = false;
        }
        else {
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
                    const std::wstring artist = x.variousArtists
                        ? L"various artists"
                        : x.artists.front();

                    return toLower(artist + L" - " + x.title);
                };

            return key(a) < key(b);
        }
    );
}

void Library::scanFolderRecursive(const std::wstring& folder)
{
    const fs::path root(folder);

    std::error_code ec;

    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        return;
    }

    fs::recursive_directory_iterator it(
        root,
        fs::directory_options::skip_permission_denied,
        ec
    );

    for (; it != fs::recursive_directory_iterator(); it.increment(ec))
    {
        if (ec || !it->is_regular_file(ec)) {
            continue;
        }

        const fs::path& path = it->path();

        if (!path.has_extension() || !isAudioFile(path.extension().wstring())) {
            continue;
        }

        TagLib::FileRef ref(path.c_str());

        if (ref.isNull()) {
            continue;
        }

        const std::wstring artist = extractArtistLiteral(ref);

        if (artist.empty()) {
            continue;
        }

        Track track{};
        track.path = path.wstring();
        track.artists = { artist };

        if (ref.tag()) {
            auto* tag = ref.tag();

            track.title = trim(tag->title().toWString());
            track.trackNo = tag->track();
            track.album = trim(tag->album().toWString());
        }

        if (track.title.empty()) {
            continue;
        }

        if (track.trackNo == 0) {
            if (track.album.empty() || track.album == track.title) { // for safety
                track.trackNo = 1;
            }
            else {
                continue;
            }
        }

        if (track.album.empty()) {
            track.album = track.title;
        }

        addTrack(std::move(track));
    }
}

void Library::addTrack(Track&& track)
{
    const std::wstring key = toLower(track.album);
    const auto it = albumIndex.find(key);

    if (it == albumIndex.end()) {
        Album album;
        album.title = track.album;
        album.tracks.push_back(std::move(track));

        const size_t index = albums.size();

        albums.push_back(std::move(album));
        albumIndex.emplace(key, index);
    }
    else {
        albums[it->second].tracks.push_back(std::move(track));
    }
}