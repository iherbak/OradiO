#ifndef WIFI_h
#define WIFI_h

#include <esp_wifi.h>
#include <string>

#define WIFI_TAG "Wifi"
class Wifi
{
public:
    Wifi();
    void InitAsSTA(const char *ssid, const char *password);
    void InitAsAP(const char *ssid, const char *password);
    esp_err_t Start(esp_event_handler_t eventHandler, void *arg);
    esp_err_t Connect();
    esp_err_t Disconnect();
    wifi_mode_t mode;
private:
    void Init(const char *ssid, const char *password, wifi_mode_t startupMode, wifi_auth_mode_t authModem);
    wifi_config_t _wifiConfig;
    wifi_mode_t _startupMode;
};

#endif
