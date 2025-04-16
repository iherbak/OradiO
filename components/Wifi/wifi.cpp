#include "wifi.h"
#include <string.h>
#include <esp_log.h>
#include <vector>

Wifi::Wifi() {
};

void Wifi::Init(const char *ssid, const char *password, wifi_mode_t startupMode, wifi_auth_mode_t authMode)
{
    _startupMode = startupMode;

    if (startupMode == WIFI_MODE_STA)
    {
        _wifiConfig = {
            .sta = {
                .scan_method = WIFI_FAST_SCAN,
                .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,

                .threshold = {
                    .rssi = -127,
                    .authmode = authMode},
                .failure_retry_cnt = 3,
            },
        };
        // copying over ssid and password
        memset(_wifiConfig.sta.ssid, '\0', sizeof(_wifiConfig.sta.ssid));
        memset(_wifiConfig.sta.password, '\0', sizeof(_wifiConfig.sta.password));
        strncpy((char *)_wifiConfig.sta.ssid, ssid, strlen(ssid));
        strncpy((char *)_wifiConfig.sta.password, password, strlen(password));
    }
    if (startupMode == WIFI_MODE_AP)
    {
        _wifiConfig = {
            .ap = {
                .channel = 7,
                .authmode = authMode,
                .max_connection = 3,
                .pmf_cfg = {
                    .required = true}},
        };
        memset(_wifiConfig.ap.ssid, '\0', sizeof(_wifiConfig.ap.ssid));
        memset(_wifiConfig.ap.password, '\0', sizeof(_wifiConfig.ap.password));
        strncpy((char *)_wifiConfig.ap.ssid, ssid, strlen(ssid));
        strncpy((char *)_wifiConfig.ap.password, password, strlen(password));
    }
};

void Wifi::InitAsSTA(const char *ssid, const char *password)
{
    Init(ssid, password, WIFI_MODE_STA, WIFI_AUTH_WPA2_PSK);
};

void Wifi::InitAsAP(const char *ssid, const char *password)
{
    Init(ssid, password, WIFI_MODE_AP, WIFI_AUTH_WPA2_PSK);
};

esp_err_t Wifi::Start(esp_event_handler_t eventHandler, void *arg)
{
    ESP_LOGI(WIFI_TAG, "default station init");
    // Initialize default station as network interface instance (esp-netif)
    if (_startupMode == WIFI_MODE_STA)
    {
        esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
        assert(sta_netif);
    }
    if (_startupMode == WIFI_MODE_AP)
    {
        esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
        assert(ap_netif);
    }

    ESP_LOGI(WIFI_TAG, "wifi init");
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, eventHandler, arg, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, ESP_EVENT_ANY_ID, eventHandler, arg, NULL));

    ESP_LOGI(WIFI_TAG, "The SSID %s", _wifiConfig.sta.ssid);
    ESP_LOGI(WIFI_TAG, "The pass %s", _wifiConfig.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(_startupMode));
    ESP_ERROR_CHECK(esp_wifi_set_config(_startupMode == WIFI_MODE_STA ? WIFI_IF_STA : WIFI_IF_AP, &_wifiConfig));
    return ESP_OK;
}

esp_err_t Wifi::Connect()
{
    ESP_ERROR_CHECK(esp_wifi_start());
    return ESP_OK;
};

esp_err_t Wifi::Disconnect()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    return ESP_OK;
};