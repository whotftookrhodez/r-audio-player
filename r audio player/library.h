#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct Track {
    std::vector<std::wstring> artists;
    std::wstring path;
    std::wstring title;
    std::wstring album;

    unsigned int trackNo = 0;
};

struct Album {
    std::vector<std::wstring> artists;
    bool variousArtists = false;

    std::wstring title;
    std::vector<Track> tracks;
};

class Library {
public:
    void scan(const std::vector<std::wstring>& roots);

    const std::vector<Album>& getAlbums() const { return albums; }
private:
    static bool isAudioFile(const std::wstring& ext);

    std::vector<Album> albums;
    std::unordered_map<std::wstring, size_t> albumIndex;

    void scanFolderRecursive(const std::wstring& folder);
    void addTrack(Track&& track);
};