#pragma once
#include "displaySettings.h"
#include "vs1053Settings.h"
#include "wifiSettings.h"

class OSettings
{
public:
    DisplaySettings display;
    Vs1053Settings vs1053;
    WifiSettings wifi;
    OSettings() { }
    OSettings(DisplaySettings d, Vs1053Settings vs, WifiSettings w){
        display = d;
        vs1053 = vs;
        wifi = w;
    }
};