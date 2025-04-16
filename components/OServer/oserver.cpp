#include "oserver.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/apps/netbiosns.h"
#include "handlers.h"
#include "../OFileSystem/ofileSystem.h"

esp_err_t OServer::start_rest_server(stationChangeCallback stationChange, volumeSetCallback volumeSet, reloadPlaylistCallback reloadPlaylist, restartCallback restart)
{
    _serverContext = (Context *)(calloc(1, sizeof(Context)));
    if(_serverContext == nullptr)
    {
        ESP_LOGI(OSERVER_TAG,"Not enough memory for server!!!!");
    }
    _serverContext->stationChange = stationChange;
    _serverContext->volumeChange = volumeSet;
    _serverContext->reloadPlaylist = reloadPlaylist;
    _serverContext->restart = restart;
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(OSERVER_TAG, "Starting HTTP Server");
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    /*
     *   POST HANDLERS
     */

    httpd_uri_t changeStation_post_uri = {
        .uri = "/api/setstation",
        .method = HTTP_POST,
        .handler = Handlers::changeStation_post_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &changeStation_post_uri);

    httpd_uri_t setVolume_post_uri = {
        .uri = "/api/setvolume",
        .method = HTTP_POST,
        .handler = Handlers::setVolume_post_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &setVolume_post_uri);

    httpd_uri_t settings_post_uri = {
        .uri = "/api/settings",
        .method = HTTP_POST,
        .handler = Handlers::settings_post_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &settings_post_uri);

    httpd_uri_t playlist_post_uri = {
        .uri = "/api/playlist",
        .method = HTTP_POST,
        .handler = Handlers::playlist_post_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &playlist_post_uri);

    httpd_uri_t restart_post_uri = {
        .uri = "/api/restart",
        .method = HTTP_POST,
        .handler = Handlers::restart_post_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &restart_post_uri);
    /*
     *   GET HANDLERS
     */
    /* URI handler for fetching system info */
    httpd_uri_t system_playlist_get_uri = {
        .uri = "/api/playlist",
        .method = HTTP_GET,
        .handler = Handlers::playlist_get_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &system_playlist_get_uri);

    httpd_uri_t system_settings_get_uri = {
        .uri = "/api/settings",
        .method = HTTP_GET,
        .handler = Handlers::settings_get_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &system_settings_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = Handlers::rest_common_get_handler,
        .user_ctx = _serverContext};
    httpd_register_uri_handler(server, &common_get_uri);
    started = true;
    return ESP_OK;
};

esp_err_t OServer::start(stationChangeCallback stationChange, volumeSetCallback volumeSet, reloadPlaylistCallback reloadPlaylist, restartCallback restart)
{
    netbiosns_init();
    netbiosns_set_name(MDNS_HOST_NAME);
    ESP_ERROR_CHECK(start_rest_server(stationChange, volumeSet, reloadPlaylist, restart));
    return ESP_OK;
};

esp_err_t OServer::stop()
{
    free(_serverContext);
    started = false;
    return ESP_OK;
};