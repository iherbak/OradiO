#pragma once

#include <string>
#include "freertos/FreeRTOS.h"
#include "esp_http_client.h"
#define STREAMREADER_TAG "Streamreader"
#define MAX_INPUT_BUFFER 2048
#define RING_BUFFER_SIZE 100000
#define MIN_RING_BUFFER_SIZE  RING_BUFFER_SIZE * 0.6
#define MAX_RING_BUFFER_SIZE  RING_BUFFER_SIZE * 0.95

struct domainAndPort
{
    std::string domain;
    std::string path;
    std::string port;
};

class StreamReader
{

public:
    StreamReader();
    esp_err_t connectToHostHttpClient(char *radioUrl, http_event_handle_cb readerCb);
    esp_err_t getHeadersHttpClient();
    esp_err_t closeConnectionHttpClient();
    int readDataHttpClient(char *recv_buff, int requestedBytes);
    void closeStream();
    bool connected = false;

private:
    std::string urlEncode(const std::string &str);
    int port = 80;
    QueueHandle_t _streamDataQueue;
    esp_http_client_handle_t client;
};
