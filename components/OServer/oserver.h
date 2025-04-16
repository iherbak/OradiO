#pragma once

#include <esp_http_server.h>
#include <string.h>
#include "esp_chip_info.h"
#include "esp_vfs.h"
#include "context.h"
#include <esp_err.h>

#define MDNS_HOST_NAME "ORadio"
#define OSERVER_TAG "OServer"

class OServer
{
public:
    esp_err_t start(stationChangeCallback stationChange, volumeSetCallback volumeSet, reloadPlaylistCallback reloadPlaylist, restartCallback restart);
    esp_err_t stop();
    bool started;

private:
    esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath);
    esp_err_t start_rest_server(stationChangeCallback stationChange, volumeSetCallback volumeSet, reloadPlaylistCallback reloadPlaylist, restartCallback restart);
    Context *_serverContext;
};
