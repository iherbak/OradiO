#pragma once

#include "ArduinoJson.h"
#include <string.h>
#include "playlist.h"
#include <map>
#include "./settings/displaySettings.h"
#include "./settings/vs1053Settings.h"
#include "./settings/oSettings.h"

namespace ArduinoJson
{
    template <>
    struct Converter<Station>
    {
        static bool toJson(const Station &src, JsonVariant dst)
        {
            dst["id"] = src.id;
            dst["url"] = src.url;
            dst["desc"] = src.desc;
            return true;
        }

        static Station fromJson(JsonVariantConst src)
        {
            return Station(src["id"], src["url"], src["desc"]);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["id"].is<int>() && src["url"].is<std::string>() && src["desc"].is<std::string>();
        }
    };

    template <>
    struct Converter<Playlist>
    {
        static bool toJson(const Playlist &src, JsonVariant dst)
        {
            dst["defaultStation"] = src.defaultStation;
            JsonDocument doc;
            JsonArray arr = doc.to<JsonArray>();
            for (auto s : src.stations)
            {
                arr.add(s.second);
            }
            dst["stations"] = arr;
            return true;
        }

        static Playlist fromJson(JsonVariantConst src)
        {
            std::map<int, Station> stations;
            JsonArrayConst arr = src["stations"].as<JsonArrayConst>();
            for (Station s : arr)
            {
                stations[s.id] = s;
            }
            return Playlist(src["defaultStation"].as<int>(), stations);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["id"].is<int>() && src["url"].is<std::string>() && src["desc"].is<std::string>();
        }
    };

    template <>
    struct Converter<DisplaySettings>
    {
        static bool toJson(const DisplaySettings &src, JsonVariant dst)
        {
            dst["sdapin"] = (uint8_t)src.sdapin;
            dst["sclpin"] = (uint8_t)src.sclpin;
            dst["rows"] = src.rows;
            dst["cols"] = src.cols;
            return true;
        }

        static DisplaySettings fromJson(JsonVariantConst src)
        {
            return DisplaySettings(src["sdapin"], src["sclpin"], src["rows"], src["cols"]);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["sdapin"].is<uint8_t>() && src["sclpin"].is<uint8_t>() && src["rows"].is<uint8_t>() && src["cols"].is<uint8_t>();
        }
    };

    template <>
    struct Converter<Vs1053Settings>
    {
        static bool toJson(const Vs1053Settings &src, JsonVariant dst)
        {
            dst["dreqpin"] = (uint8_t)src.dreqpin;
            dst["xcspin"] = (uint8_t)src.xcspin;
            dst["xdcspin"] = (uint8_t)src.xdcspin;
            dst["resetpin"] = (uint8_t)src.resetpin;
            dst["mosipin"] = (uint8_t)src.mosipin;
            dst["misopin"] = (uint8_t)src.misopin;
            dst["sclkpin"] = (uint8_t)src.sclkpin;
            dst["startvolume"] = (uint8_t)src.startvolume;

            return true;
        }

        static Vs1053Settings fromJson(JsonVariantConst src)
        {
            return Vs1053Settings(src["dreqpin"], src["xcspin"], src["xdcspin"], src["reset"], src["mosipin"], src["misopin"], src["sclkpin"], src["startvolume"]);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["dreqpin"].is<uint8_t>() &&
                   src["xcspin"].is<uint8_t>() &&
                   src["xdcspin"].is<uint8_t>() &&
                   src["resetpin"].is<uint8_t>() &&
                   src["mosipin"].is<uint8_t>() &&
                   src["misopin"].is<uint8_t>() &&
                   src["sclkpin"].is<uint8_t>() &&
                   src["startvolume"].is<uint8_t>();
        }
    };

    template <>
    struct Converter<OSettings>
    {
        static bool toJson(const OSettings &src, JsonVariant dst)
        {
            dst["display"] = src.display;
            dst["vs1053"] = src.vs1053;
            dst["wifi"] = src.wifi;
            return true;
        }

        static OSettings fromJson(JsonVariantConst src)
        {
            return OSettings(src["display"], src["vs1053"], src["wifi"]);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["display"].is<DisplaySettings>() && src["vs1053"].is<Vs1053Settings>();
        }
    };

    template <>
    struct Converter<WifiSettings>
    {
        static bool toJson(const WifiSettings &src, JsonVariant dst)
        {
            dst["stassid"] = src.stassid;
            dst["stapassword"] = src.stapassword;
            dst["apssid"] = src.apssid;
            dst["appassword"] = src.appassword;
            dst["startasap"] = src.startasap;
            return true;
        }

        static WifiSettings fromJson(JsonVariantConst src)
        {
            return WifiSettings(src["stassid"], src["stapassword"], src["apssid"], src["appassword"],src["startasap"]);
        }

        static bool checkJson(JsonVariantConst src)
        {
            return src["stassid"].is<std::string>() && src["stapassword"].is<std::string>() && src["apssid"].is<std::string>() && src["appassword"].is<std::string>() && src["startasp"].is<bool>();
        }
    };
}
