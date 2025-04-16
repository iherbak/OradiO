#pragma once

#include <string>
#include <map>
#include "station.h"

class Playlist
{
public:
    int defaultStation;
    int stationsCount = 0;
    std::map<int, Station> stations;

    Playlist() {}

    Playlist(int defaultId, std::map<int, Station> &stat)
    {
        defaultStation = defaultId;
        stations = stat;
        stationsCount = stat.size();
    }
};