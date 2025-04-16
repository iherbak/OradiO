#include "streamReader.h"
#include <esp_log.h>
#include <sys/param.h>
#include <string.h>
#include <string>
#include <bits/stdc++.h>
#include <iostream>
#include <iomanip>
#include <lwip/netdb.h>
#include <regex.h>
#include "driver/i2s_std.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

StreamReader::StreamReader() {
};

esp_err_t StreamReader::getHeadersHttpClient()
{
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length > 0)
    {
        return ESP_OK;
    }
    return ESP_FAIL;
}

int StreamReader::readDataHttpClient(char *recv_buff, int requestedBytes)
{
    return esp_http_client_read(client, recv_buff, requestedBytes);
}

esp_err_t StreamReader::connectToHostHttpClient(char *radioUrl, http_event_handle_cb readerCb)
{
    esp_http_client_config_t config = {
        .url = radioUrl,
        .event_handler = readerCb,
        .buffer_size = MAX_INPUT_BUFFER,
        .keep_alive_enable = true};
    if (tolower(radioUrl[0]) == 'h' && tolower(radioUrl[1]) == 't' && tolower(radioUrl[2]) == 't' && tolower(radioUrl[3]) == 'p' && tolower(radioUrl[4]) == 's')
    {
        config.crt_bundle_attach = esp_crt_bundle_attach;
    }

    client = esp_http_client_init(&config);
    // request icy metadata in stream
    esp_http_client_set_header(client, "accept", "*/*");
    esp_http_client_set_header(client, "icy-metadata", "1");
    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK)
    {
        ESP_LOGE(STREAMREADER_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return ESP_FAIL;
    }
    connected = true;
    return ESP_OK;
}

esp_err_t StreamReader::closeConnectionHttpClient()
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    connected = false;
    return ESP_OK;
}