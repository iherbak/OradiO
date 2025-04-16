#pragma once

#define HANDLERS_TAG "Handlers"
#include <esp_err.h>
#include <esp_http_server.h>
#include "context.h"
#include <string>
#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

class Handlers{
    public:
    static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath);
    static esp_err_t rest_common_get_handler(httpd_req_t *req);
    static esp_err_t playlist_get_handler(httpd_req_t *req);
    static esp_err_t playlist_post_handler(httpd_req_t *req);
    static esp_err_t settings_get_handler(httpd_req_t *req);
    static esp_err_t settings_post_handler(httpd_req_t *req);
    static esp_err_t restart_post_handler(httpd_req_t *req);
    static esp_err_t changeStation_post_handler(httpd_req_t *req);
    static esp_err_t setVolume_post_handler(httpd_req_t *req);
    static void enableGzipped(httpd_req_t *req, std::string &path);
    static esp_err_t SendFileChunked(httpd_req_t *req, std::string &filePath);
    static esp_err_t RecieveFileChunked(httpd_req_t *req, std::string &filePath);
};
