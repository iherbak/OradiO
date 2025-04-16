#include <string>
#pragma once

class WifiSettings
{
public:
    std::string stassid;
    std::string stapassword;
    std::string apssid;
    std::string appassword;
    bool startasap;
    
    WifiSettings() {}
    WifiSettings(std::string stas, std::string stap, std::string aps, std::string app, bool startas)
    {
        ESP_LOGI("jext", "%s - %s", stas.c_str(), stap.c_str());
        stassid = stas;
        stapassword = stap;
        apssid = aps;
        appassword = app;
        startasap = startas;
    }
};