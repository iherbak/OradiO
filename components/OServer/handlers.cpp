#include "handlers.h"
#include <string>
#include <esp_log.h>
#include <esp_chip_info.h>
#include <sys/_default_fcntl.h>
#include <fstream>
#include "../OFileSystem/ofileSystem.h"

void Handlers::enableGzipped(httpd_req_t *req, std::string &path)
{
    if (path.ends_with(".js") ||
        path.ends_with(".html") ||
        path.ends_with(".css") ||
        path.ends_with(".png") ||
        path.ends_with(".svg") ||
        path.ends_with(".ico"))
    {
        path += ".gz";
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    };
}

esp_err_t Handlers::SendFileChunked(httpd_req_t *req, std::string &filePath)
{
    char *buffer = new char[SCRATCH_BUFSIZE];
    std::string path = FS_MOUNT_POINT + filePath;
    enableGzipped(req, path);
    std::ifstream fs(path);
    if (fs.good())
    {
        // ESP_LOGE(HANDLERS_TAG, "Opened file: %s", path.c_str());
        set_content_type_from_file(req, filePath.c_str());
        int got = 1;
        while (got > 0)
        {
            got = OFileSystem::readFromFileStream(buffer, fs, SCRATCH_BUFSIZE);
            if (got > 0)
            {
                httpd_resp_send_chunk(req, buffer, got);
            }
        }
        // ESP_LOGI(HANDLERS_TAG, "End of file");
        fs.close();
        httpd_resp_send_chunk(req, NULL, 0);
    }
    else
    {
        ESP_LOGE(HANDLERS_TAG, "Failed to open file : %s", path.c_str());
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }
    delete[] buffer;
    buffer = nullptr;
    return ESP_OK;
}

esp_err_t Handlers::RecieveFileChunked(httpd_req_t *req, std::string &filePath)
{
    char *buffer = new char[SCRATCH_BUFSIZE];
    int total_len = req->content_len;
    std::string path = FS_MOUNT_POINT + filePath;
    std::ofstream fs(path, std::ofstream::trunc);
    if (fs.good())
    {
        ESP_LOGE(HANDLERS_TAG, "Opened file for write: %s", path.c_str());
        int requestAmount = total_len < SCRATCH_BUFSIZE ? total_len : SCRATCH_BUFSIZE;
        int got = 0;
        while (got < total_len)
        {
            got += httpd_req_recv(req, buffer, requestAmount);
            OFileSystem::writeToFileStream(buffer, fs, got);
        }
        ESP_LOGI(HANDLERS_TAG, "End of file");
        fs.flush();
        fs.close();
    }
    else
    {
        ESP_LOGE(HANDLERS_TAG, "Failed to open file : %s", path.c_str());
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write existing file");
        return ESP_FAIL;
    }
    delete[] buffer;
    buffer = nullptr;
    httpd_resp_sendstr(req, "");

    return ESP_OK;
}

/* Set HTTP response content type according to file extension */
esp_err_t Handlers::set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html"))
    {
        type = "text/html";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".js"))
    {
        type = "application/javascript";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".css"))
    {
        type = "text/css";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".png"))
    {
        type = "image/png";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".ico"))
    {
        type = "image/x-icon";
    }
    else if (CHECK_FILE_EXTENSION(filepath, ".svg"))
    {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
};

esp_err_t Handlers::rest_common_get_handler(httpd_req_t *req)
{
    std::string filePath;
    if (req->uri[strlen(req->uri) - 1] == '/')
    {
        filePath = "/index.html";
    }
    else
    {
        filePath = req->uri;
    }
    SendFileChunked(req, filePath);
    return ESP_OK;
};

esp_err_t Handlers::playlist_get_handler(httpd_req_t *req)
{
    std::string filePath = "/playlist.json";
    return SendFileChunked(req, filePath);
}

esp_err_t Handlers::settings_get_handler(httpd_req_t *req)
{
    std::string filePath = "/settings.json";
    return SendFileChunked(req, filePath);
}

esp_err_t Handlers::settings_post_handler(httpd_req_t *req)
{
    std::string path = "/settings.json";
    RecieveFileChunked(req, path);
    return ESP_OK;
}

esp_err_t Handlers::playlist_post_handler(httpd_req_t *req)
{
    Context *_serverContext = (Context *)(req->user_ctx);
    std::string path = "/playlist.json";
    RecieveFileChunked(req, path);
    _serverContext->reloadPlaylist();
    return ESP_OK;
}

esp_err_t Handlers::setVolume_post_handler(httpd_req_t *req)
{
    Context *_serverContext = (Context *)(req->user_ctx);

    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = _serverContext->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    ESP_LOGI(HANDLERS_TAG, "Setting volume to %s", buf);
    bool up = true;
    if (buf[0] == 'f')
    {
        up = false;
    }
    _serverContext->volumeChange(up);
    httpd_resp_sendstr(req, "Changing station");
    return ESP_OK;
}

esp_err_t Handlers::restart_post_handler(httpd_req_t *req){
    Context *_serverContext = (Context *)(req->user_ctx);
    httpd_resp_sendstr(req, "Restarting");
    _serverContext->restart();
    return ESP_OK;
}
esp_err_t Handlers::changeStation_post_handler(httpd_req_t *req)
{
    Context *_serverContext = (Context *)(req->user_ctx);

    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = _serverContext->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    int channelId = std::stoi(buf, 0, 10);
    ESP_LOGI(HANDLERS_TAG, "Changing station requested to %d", channelId);
    _serverContext->stationChange(channelId);
    httpd_resp_sendstr(req, "Changing station");
    return ESP_OK;
}
