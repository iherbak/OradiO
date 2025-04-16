#pragma once
#include <string>

class Station
{
public:
    int id;
    std::string url;
    std::string desc;
    Station()
    {
    }
    Station(int i, std::string u, std::string d)
    {
        id = i;
        url = u;
        desc = d;
    }
};