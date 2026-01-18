#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

struct Track
{
    unsigned int trackNo = 0;

    std::string album;
    std::vector<std::string> artists;
    std::string path;
    std::string title;
};

struct Album
{
    bool variousArtists = false;

    std::vector<std::string> artists;
    std::string title;
    std::vector<Track> tracks;
};

class Library
{
public:
    void scan(const std::vector<std::filesystem::path>& roots);

    const std::vector<Album>& getAlbums() const { return albums; }
private:
    std::vector<Album> albums;
    std::unordered_map<std::string, size_t> albumIndex;

    void scanFolderRecursive(const std::filesystem::path& folder);
    void addTrack(Track&& track);
};