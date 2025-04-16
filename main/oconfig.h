#pragma once

#include <string>
#include <string.h>
#include "./models/playlist.h"
#include "ArduinoJson.h"
#include "./models/jsonExtensions.h"
#include "./models/settings/oSettings.h"

static Playlist ParseJsonPlaylist(std::string fileContent)
{
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, fileContent);
    auto pl = jsonDoc.as<Playlist>();
    return pl;
}

static OSettings ParseJsonSettings(std::string fileContent)
{
    JsonDocument jsonDoc;
    deserializeJson(jsonDoc, fileContent);
    auto pl = jsonDoc.as<OSettings>();
    return pl;
}

static std::string SettingsToString(OSettings settings)
{
    char * buffer = new char[700];
    memset(buffer,'\0',700);
    JsonDocument doc;
    doc.set<OSettings>(settings);
    serializeJson(doc.as<JsonVariantConst>(), buffer,700);
    std::string s(buffer);
    return s;
}