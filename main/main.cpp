#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <driver/i2c_master.h>
#include "../components/Wifi/wifi.h"
#include "../components/StreamReader/streamReader.h"
#include "../components/OFileSystem/ofileSystem.h"
#include "./models/displayMessage.h"
#include "./models/displaySegment.h"
#include <vector>
#include "display.h"
#include <string>
#include <bits/stdc++.h>
#include <sys/param.h>
#include "../components/OServer/oserver.h"
#include "../components/Vs1053/vs1053.h"
#include "esp_event.h"
#include "freertos/ringbuf.h"
#include <regex.h>
#include "main.h"
#include "oconfig.h"
#include "./models/vs1503Command.h"

#define TASK_ARRAY_START_NOTFIY 0
#define TASK_ARRAY_BITRATE_NOTIFY 1
#define TASK_ARRAY_RESET_NOTIFY 2
#define TASK_ARRAY_WIFI_NOT_CONNECTED 3
#define TASK_ARRAY_WIFI_CONNECTED 4
#define TASK_ARRAY_GRACEFUL_RESTART 5
#define READ_MAX_RETRY 5
#define STREAM_CHUNK_SIZE 64

TaskHandle_t mainTaskHandle = NULL;
TaskHandle_t streamingTaskHandle = NULL;
TaskHandle_t fetcherTaskHandle = NULL;

QueueHandle_t stationQueue;
QueueHandle_t commandQueue;

OServer oServer;
Display *display;
SemaphoreHandle_t ringbufferSemaphore = NULL;
RingbufHandle_t ringBuffer;
OSettings oSettings;

int lastStation = 0;
int metaBitrate = 192;
int metaInt = 0;
int volume = CONFIG_VOLUME;
bool mainReset = false;

uint8_t prevStationButton = 0;
uint8_t nextStationButton = 1;
uint8_t volumeDownButton = 2;
uint8_t volumeUpButton = 3;
VS1053 vs1503Device;

Wifi wifi;
double streamTaskDelay;

void buttonHandler(void *arg)
{
    uint8_t *buttonNum = (uint8_t *)arg;
    uint8_t b = *buttonNum;
    Vs1053Commands volumeCommand;
    switch (b)
    {
    case 0:
    {
        if (lastStation - 1 >= 0)
        {
            lastStation--;
            xQueueSendFromISR(stationQueue, &lastStation, NULL);
        }
        break;
    }
    case 1:
    {
        if (lastStation + 1 < playList.stationsCount)
        {
            lastStation++;
            xQueueSendFromISR(stationQueue, &lastStation, NULL);
        }
        break;
    }
    case 2:
    {
        if (volume - 1 > 0)
        {
            volumeCommand = VOLUMEDOWN;
            xQueueSendFromISR(commandQueue, &volumeCommand, NULL);
        }
        break;
    }
    case 3:
    {
        if (volume + 1 < 100)
        {
            volumeCommand = VOLUMEUP;
            xQueueSendFromISR(commandQueue, &volumeCommand, NULL);
        }
        break;
    }
    }
}

void setupButton(gpio_num_t gp, uint8_t *number)
{
    gpio_reset_pin(gp);
    gpio_set_direction(gp, GPIO_MODE_INPUT);
    gpio_set_intr_type(gp, GPIO_INTR_NEGEDGE);
    gpio_intr_enable(gp);
    gpio_isr_handler_add(gp, &buttonHandler, number);
}

esp_err_t httpHandler(esp_http_client_event_t *event)
{
    std::string decoderMessage = "";
    switch (event->event_id)
    {
    case HTTP_EVENT_ON_HEADER:
    {
        // reset Metaint
        ESP_LOGI("httpev", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", event->header_key, event->header_value);
        std::string key(event->header_key);
        if (key.contains("icy-name"))
        {
            decoderMessage = event->header_value;
            display->SetContent(0, decoderMessage);
        }
        if (key.contains("icy-description"))
        {
            decoderMessage = event->header_value;
            display->SetContent(1, decoderMessage);
        }
        if (key.contains("icy-br"))
        {
            metaBitrate = atoi(event->header_value);
        }
        if (key.contains("icy-metaint"))
        {
            metaInt = atoi(event->header_value);
        }
        break;
    }
    default:
    {
        break;
    }
    }
    return ESP_OK;
}

void executeCommand(Vs1053Commands command)
{
    switch (command)
    {
    case VOLUMEDOWN:
    {
        vs1503Device.setVolume(vs1503Device.getVolume() - 1);
        volume--;
        break;
    }
    case VOLUMEUP:
    {
        volume++;
        vs1503Device.setVolume(vs1503Device.getVolume() + 1);
        break;
    }
    }
}

void streamingTask(void *param)
{
    char *taskName = pcTaskGetName(0);
    Vs1053Commands command;
    bool reset = false;
    uint32_t notificationValue;
    bool running;
    uint8_t *buffer;
    size_t size;
    int request = MAX_INPUT_BUFFER * 2;
    int bitrate = 192;
    while (!reset)
    {
        running = true;
        ESP_LOGI(taskName, "Waiting for start notification");
        // wait for start notification
        xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
        switch (notificationValue)
        {
        case TASK_ARRAY_GRACEFUL_RESTART:
        {
            reset = true;
            break;
        }
        case TASK_ARRAY_RESET_NOTIFY:
        {
            ESP_LOGI(taskName, "Stopping streamin");
            vs1503Device.stopSong();
            vs1503Device.clearDecodedTime();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            xTaskNotify(mainTaskHandle, TASK_ARRAY_START_NOTFIY, eSetValueWithoutOverwrite);
            break;
        }
        case TASK_ARRAY_START_NOTFIY:
        {
            ESP_LOGI(taskName, "Got notification");
            // initial burst to get bitrate
            ESP_LOGI(taskName, "Initial burst start");
            xSemaphoreTake(ringbufferSemaphore, portMAX_DELAY);
            buffer = (uint8_t *)xRingbufferReceiveUpTo(ringBuffer, &size, portMAX_DELAY, request);
            xSemaphoreGive(ringbufferSemaphore);
            vs1503Device.playChunk(buffer, size);
            vRingbufferReturnItem(ringBuffer, (void *)buffer);
            uint16_t hdat0 = vs1503Device.readHDat0();
            uint16_t hdat1 = vs1503Device.readHDat1();
            ESP_LOGI(taskName, "HDAT0 %02x HDAT1 %02x", hdat0, hdat1);
            if (hdat0 == 0)
            {
                bitrate = metaBitrate;
            }
            request = STREAM_CHUNK_SIZE;
            // calculate taskdelay to feed fluently
            streamTaskDelay = ((double)(STREAM_CHUNK_SIZE * 8) / (double)(bitrate * 1000)) * 1000;
            ESP_LOGI(taskName, "Bitrate calculated notifying fetcher");
            xTaskNotify(fetcherTaskHandle, STREAM_CHUNK_SIZE, eSetValueWithoutOverwrite);
            ESP_LOGI(taskName, "Bitrate is %d taskdelay for %d bytes %.1f ms", bitrate, STREAM_CHUNK_SIZE, streamTaskDelay);
            // std::string r("fdws");
            // dev.printDetails(r);
            while (running)
            {
                xSemaphoreTake(ringbufferSemaphore, portMAX_DELAY);
                buffer = (uint8_t *)xRingbufferReceiveUpTo(ringBuffer, &size, 100 / portTICK_PERIOD_MS, request);
                xSemaphoreGive(ringbufferSemaphore);
                // if no data skip play, maybe fetcher is lagging behind, if so it will restart connecting
                if (size > 0 && buffer != nullptr)
                {
                    // ESP_LOGI(taskName, "Reciving from buffer %d", request);
                    vs1503Device.playChunk(buffer, size);
                    vRingbufferReturnItem(ringBuffer, (void *)buffer);
                }
                if (xQueueReceive(commandQueue, &command, 0))
                {
                    // ESP_LOGI(taskName,"Executing command %d",command);
                    executeCommand(command);
                }
                if (pdPASS == xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, 0))
                {
                    switch (notificationValue)
                    {
                    case TASK_ARRAY_GRACEFUL_RESTART:
                    {
                        reset = true;
                    }
                    case TASK_ARRAY_RESET_NOTIFY:
                    {
                        running = false;
                    }
                    }
                }
                vTaskDelay(streamTaskDelay / portTICK_PERIOD_MS);
            }
            ESP_LOGI(taskName, "Stopping streamin");
            vs1503Device.stopSong();
            vs1503Device.clearDecodedTime();
            vTaskDelay(100 / portTICK_PERIOD_MS);
            xTaskNotify(mainTaskHandle, TASK_ARRAY_START_NOTFIY, eSetValueWithoutOverwrite);
            break;
        }
        }
    }
    xTaskNotify(mainTaskHandle, TASK_ARRAY_START_NOTFIY, eSetValueWithoutOverwrite);
    ESP_LOGI(taskName, "Shutting down");
    vTaskDelete(streamingTaskHandle);
}

void getMetaStreamTitle(std::string &icyMeta)
{
    regex_t titleRegex;

    int reti;
    reti = regcomp(&titleRegex, "StreamTitle='(.*)'", REG_EXTENDED | REG_ICASE);
    if (reti)
    {
        ESP_LOGE(STREAMREADER_TAG, "Error compiling regexp");
    }
    else
    {
        // we are looking for 2 submatch 0 is always the largest match
        regmatch_t match[2];
        regmatch_t *matchp = match;
        reti = regexec(&titleRegex, icyMeta.c_str(), 3, matchp, 0);
        if (!reti)
        {
            // only match[1] is interesting 0 is the full
            // ESP_LOGI("icymeta", "Match from %lu - %lu length is %d", match[1].rm_so, match[1].rm_eo, icyMeta.length());
            icyMeta = icyMeta.substr(match[1].rm_so, match[1].rm_eo - match[1].rm_so);
        }
        else
        {
            icyMeta = "Various";
            ESP_LOGE(STREAMREADER_TAG, "No matching parts in url");
        }
    }
    regfree(&titleRegex);
}

void fetchTask(void *param)
{
    char *taskName = pcTaskGetName(0);
    bool reset = false;
    uint32_t notificationValue;
    StreamReader sr;
    uint8_t *buffer = new uint8_t[MAX_INPUT_BUFFER];
    std::string icymeta;
    int read = 0;
    int request = MAX_INPUT_BUFFER;
    size_t freeSize = 0;
    bool running;
    int readSum = 0;
    int readMeta = 0;
    uint8_t metaLength = 0;
    while (!reset)
    {
        running = true;
        ESP_LOGI(taskName, "Waiting notification");
        // wait for start notification
        xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
        ESP_LOGI(taskName, "Got notification %ld", notificationValue);
        switch (notificationValue)
        {
        case TASK_ARRAY_GRACEFUL_RESTART:
        {
            // we are not yet started or in AP mode
            reset = true;
            xTaskNotify(streamingTaskHandle, TASK_ARRAY_GRACEFUL_RESTART, eSetValueWithoutOverwrite);
            break;
        }
        case TASK_ARRAY_RESET_NOTIFY:
        {
            // signal back to main to emulate successfull restarting as we are not even started
            xTaskNotify(streamingTaskHandle, TASK_ARRAY_RESET_NOTIFY, eSetValueWithoutOverwrite);
            break;
        }
        case TASK_ARRAY_START_NOTFIY:
        {
            ESP_LOGI(taskName, "Got start notification");

            // no concurrent acces yet as we are about to start
            freeSize = xRingbufferGetCurFreeSize(ringBuffer);

            // empty buffer before start for smnooth song change
            if (freeSize != RING_BUFFER_SIZE)
            {
                size_t readout = 0;
                // ESP_LOGI(taskName, "Ringbuffer has left %d", RING_BUFFER_SIZE - freeSize);
                while (freeSize != RING_BUFFER_SIZE)
                {
                    // get out everything and give back to clear buffer
                    uint8_t *fullbuff = (uint8_t *)xRingbufferReceive(ringBuffer, &readout, portMAX_DELAY);
                    vRingbufferReturnItem(ringBuffer, (void *)fullbuff);
                    freeSize = xRingbufferGetCurFreeSize(ringBuffer);
                    ESP_LOGI(taskName, "Ringbuffer spit out %d", readout);
                }
            }

            char *url = new char[150];
            memset(url, '\0', 150);
            memmove(url, playList.stations[lastStation].url.c_str(), playList.stations[lastStation].url.length());
            ESP_LOGI(taskName, "Connecting to %s - %d", url, playList.stations[lastStation].url.length());
            if (sr.connectToHostHttpClient(url, &httpHandler) == ESP_OK)
            {
                sr.getHeadersHttpClient();
            }
            int retry = 0;
            // initial bufferring

            while (freeSize > RING_BUFFER_SIZE - MAX_RING_BUFFER_SIZE)
            {
                read = sr.readDataHttpClient(reinterpret_cast<char *>(buffer), request);
                if (read > 0)
                {
                    if (metaInt > 0)
                    {
                        // ESP_LOGI(taskName,"MetaInt is %d",metaInt);
                        extractMeta(readSum, read, metaLength, buffer, icymeta, readMeta);
                    }
                    // ESP_LOGI(taskName, "Buffer filling: %.1f", (((double)freeSize / (double)RING_BUFFER_SIZE) * 100));
                    xRingbufferSend(ringBuffer, buffer, read, portMAX_DELAY);
                    freeSize = xRingbufferGetCurFreeSize(ringBuffer);
                }
                else
                {
                    if(read <0){
                        retry = READ_MAX_RETRY;
                    }
                    // try five times than give up
                    if (retry < READ_MAX_RETRY)
                    {
                        retry++;
                        ESP_LOGE(taskName, "Stream reading error %d pausing for a quarter sec", read);
                        vTaskDelay(250 / portTICK_PERIOD_MS);
                    }
                    else
                    {
                        std::string message("Unable to get data");
                        display->SetContent(0, message);
                        // terminate loop
                        running = false;
                        freeSize = 0;
                    }
                }
                vTaskDelay(1 / portTICK_PERIOD_MS);
            }
            if (running)
            {
                ESP_LOGI(taskName, "Notifying streaming task to start");
                xTaskNotify(streamingTaskHandle, TASK_ARRAY_START_NOTFIY, eSetValueWithoutOverwrite);
                ESP_LOGI(taskName, "Waiting for streaming task to provide it's delay interval");
                xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
            }
            while (running)
            {
                xSemaphoreTake(ringbufferSemaphore, portMAX_DELAY);
                freeSize = xRingbufferGetCurFreeSize(ringBuffer);
                xSemaphoreGive(ringbufferSemaphore);

                if (freeSize < RING_BUFFER_SIZE - MAX_RING_BUFFER_SIZE)
                {
                    double canBurn = ((double)RING_BUFFER_SIZE - (double)freeSize - (double)(MIN_RING_BUFFER_SIZE));
                    double safeDelay = streamTaskDelay * (canBurn / (double)STREAM_CHUNK_SIZE);
                    // ESP_LOGI(taskName, "Safe delay: %1.f", safeDelay);
                    vTaskDelay((safeDelay * 0.8) / portTICK_PERIOD_MS);
                }
                else
                {
                    read = sr.readDataHttpClient(reinterpret_cast<char *>(buffer), request);
                    if (read > 0)
                    {
                        if (metaInt > 0)
                        {
                            extractMeta(readSum, read, metaLength, buffer, icymeta, readMeta);
                        }
                        // ESP_LOGI(taskName, "Buffer getting low free is: %d %d", freeSize, read);
                        xRingbufferSend(ringBuffer, buffer, read, portMAX_DELAY);
                    }
                    else
                    {
                        if (retry < READ_MAX_RETRY)
                        {
                            retry++;
                            ESP_LOGI(taskName, "Stream reading error %d pausing for a sec", read);
                            vTaskDelay(250 / portTICK_PERIOD_MS);
                        }
                        else
                        {
                            std::string message("Unable to get data");
                            display->SetContent(0, message);
                            running = false;
                        }
                    }
                    vTaskDelay(1 / portTICK_PERIOD_MS);
                }
                if (pdPASS == xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, 0))
                {
                    switch (notificationValue)
                    {
                    case TASK_ARRAY_GRACEFUL_RESTART:
                    {
                        reset = true;
                    }
                    case TASK_ARRAY_RESET_NOTIFY:
                    {
                        running = false;
                    }
                    }
                }
            }
            sr.closeConnectionHttpClient();
            metaInt = 0;
            readSum = 0;
            readMeta = 0;
            metaLength = 0;
            if (retry == READ_MAX_RETRY)
            {
                ESP_LOGI(taskName, "Reiniting main to reconnectt");
                xQueueSend(stationQueue, &lastStation, 0);
            }
            else
            {
                switch (notificationValue)
                {
                case TASK_ARRAY_GRACEFUL_RESTART:
                {
                    xTaskNotify(streamingTaskHandle, TASK_ARRAY_GRACEFUL_RESTART, eSetValueWithoutOverwrite);
                }
                case TASK_ARRAY_RESET_NOTIFY:
                {
                    xTaskNotify(streamingTaskHandle, TASK_ARRAY_RESET_NOTIFY, eSetValueWithoutOverwrite);
                }
                }
            }
            delete[] url;
            url = nullptr;

            retry = 0;
            ESP_LOGI(taskName, "Stopping fetcher");
            break;
        }
        }
    }
    ESP_LOGI(taskName, "Shutting down");
    vTaskDelete(fetcherTaskHandle);
}

void extractMeta(int &readSum, int &read, uint8_t &metaLength, uint8_t *buffer, std::string &icymeta, int &readMeta)
{
    char *taskName = pcTaskGetName(0);
    readSum += read;
    /**/
    if (readSum > 0 && readMeta < metaLength)
    {
        int missing = metaLength - readMeta;
        int nowAvailable = read < missing ? read : missing;
        ESP_LOGI(taskName, "To : 0 - From % d count : % d", nowAvailable, nowAvailable);
        char *metapart = new char[nowAvailable];
        // read out available meta
        memmove(metapart, buffer, nowAvailable);
        icymeta += metapart;
        delete[] metapart;
        metapart = nullptr;
        memmove(buffer, buffer + nowAvailable, read - nowAvailable);
        readMeta += nowAvailable;
        read -= nowAvailable;
        readSum -= nowAvailable;
    }
    if (readSum > metaInt)
    {
        // ESP_LOGI(taskName, "sum %d meta limit %d read now %d", readSum, metaInt, read);
        int metaLengthBytePos = read - (readSum - metaInt);
        int metaStartBytePos = metaLengthBytePos + 1;
        int readEndBytePos = read - 1;
        metaLength = *(buffer + metaLengthBytePos);
        metaLength *= 16;
        int metaEndBytePos = metaLengthBytePos + metaLength;
        // reduce back to end
        if (metaEndBytePos > readEndBytePos)
        {
            metaEndBytePos = readEndBytePos;
        }
        int nowAvailable = metaEndBytePos - metaLengthBytePos;

        // ESP_LOGI(taskName,"length at %d start : %d  end: %d length %d available: %d", metaLengthBytePos, metaStartBytePos, metaEndBytePos, metaLength, nowAvailable);
        if (nowAvailable > 0)
        {
            // if the metasize happens to be the last byte of the read;
            // ESP_LOGI(tagName," From: %d - To %d count: %d", metaStartBytePos, metaEndBytePos, nowAvailable);
            char *metapart = new char[nowAvailable];
            // read out available meta adjust start as buffer already points to first
            memmove(metapart, buffer + metaStartBytePos, nowAvailable);
            icymeta += metapart;
            delete[] metapart;
            metapart = nullptr;
            ESP_LOGI(taskName, "%s", icymeta.c_str());
            // ESP_LOGI(taskName, "Adjusting To : %d - From %d count: %d", metaLengthBytePos, metaEndBytePos, read - metaEndBytePos);
            memmove(buffer + metaLengthBytePos, buffer + metaEndBytePos + 1, readEndBytePos - metaEndBytePos);
            readMeta += nowAvailable;
            readSum = readEndBytePos - metaEndBytePos;
            // as we got the metalength byte as well we sub one more
            read -= (nowAvailable + 1);
            // ESP_LOGI(taskName, "New Sum is %d new read %d read meta is %d %d", readSum, read, readMeta, nowAvailable);
        }
        if (nowAvailable == 0)
        {
            // no metadata content for now
            // ESP_LOGI(taskName,"Adjusting To : %d - From %d count: %d", metaLengthBytePos, metaEndBytePos, read - metaEndBytePos);
            memmove(buffer + metaLengthBytePos, buffer + metaLengthBytePos + 1, readEndBytePos - metaEndBytePos);
            readSum = readEndBytePos - metaEndBytePos;
            read--;
            // ESP_LOGI(taskName, "New Sum is %d new read %d read meta is %d %d", readSum, read, readMeta, nowAvailable);
        }
    }

    if (readMeta == metaLength && metaLength != 0)
    {
        if (icymeta != "")
        {
            getMetaStreamTitle(icymeta);
            display->SetContent(1, icymeta);
            icymeta = "";
        }
        readMeta = 0;
        metaLength = 0;
    }
}

void changeme(int channelId)
{
    char *taskName = pcTaskGetName(0);
    ESP_LOGI(taskName, "Changing to channel %d", channelId);
    if (xQueueSend(stationQueue, &channelId, 100 / portTICK_PERIOD_MS) != pdTRUE)
    {
        ESP_LOGI(taskName, "station queue is full");
    }
};

void reloadPlaylist()
{
    std::string fileContent;
    OFileSystem::readFile("playlist.json", fileContent);
    playList = ParseJsonPlaylist(fileContent);
    lastStation = playList.defaultStation;
}

void reloadSettings()
{
    std::string fileContent;
    OFileSystem::readFile("settings.json", fileContent);
    oSettings = ParseJsonSettings(fileContent);
}

void restart()
{
    ESP_LOGI("rest", "restarting");
    mainReset = true;
    xTaskNotify(mainTaskHandle, TASK_ARRAY_GRACEFUL_RESTART, eSetValueWithoutOverwrite);
}

void setVolume(bool up)
{
    char *taskName = pcTaskGetName(0);
    int v = vs1503Device.getVolume();
    ESP_LOGI(taskName, "Top level %s %d", up ? "up" : "down", v);
    Vs1053Commands volume;
    if (up)
    {
        if (v < 100)
        {
            volume = VOLUMEUP;
        }
    }
    else
    {
        if (v > 0)
        {
            volume = VOLUMEDOWN;
        }
    }
    xQueueSend(commandQueue, &volume, portMAX_DELAY);
};

void wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    char *taskName = pcTaskGetName(0);
    Display *dsp = (Display *)arg;
    std::string wifiMessage = "";
    // ESP_LOGI(taskName, "event fired %d %s", (int)event_id, event_base == WIFI_EVENT ? "wifi event" : "non wifi event");
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
        {
            esp_wifi_connect();
            ESP_LOGI(taskName, "Connect called for wifi");
            break;
        }
        case WIFI_EVENT_STA_DISCONNECTED:
        {
            wifiMessage = "Wifi disconnected...";
            dsp->SetContent(0, wifiMessage);
            ESP_LOGI(taskName, "Wifi disconnected");
            if (!mainReset)
            {
                xTaskNotify(mainTaskHandle, TASK_ARRAY_WIFI_NOT_CONNECTED, eSetValueWithoutOverwrite);
            }
            break;
        }
        case WIFI_EVENT_AP_STACONNECTED:
        {
            wifiMessage = "Connect to http://192.168.4.1";
            dsp->SetContent(1, wifiMessage);
            break;
        }
        }
    }
    if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            std::string output;
            uint32_t mask = 0xFF000000;
            for (int i = 0; i < 4; i++)
            {
                int32_t shifted = (event->ip_info.ip.addr & mask) >> (24 - i * 8);
                output = std::to_string(shifted) + "." + output;
                mask >>= 8;
            }
            wifiMessage = "IP=" + output;
            dsp->SetContent(1, wifiMessage);
            ESP_LOGI(taskName, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
            xTaskNotify(mainTaskHandle, TASK_ARRAY_WIFI_CONNECTED, eSetValueWithoutOverwrite);
            break;
        }
        }
    }
};

esp_err_t initWifi(Display *dsp)
{
    char *taskName = pcTaskGetName(0);
    std::string message("");
    ESP_LOGI(taskName, "Wifi startup");
    message = "Starting wifi...";
    dsp->SetContent(0, message);
    ESP_LOGI(taskName, "Wifi connect");
    esp_err_t ret = wifi.Start(&wifiEventHandler, dsp);
    if (ret == ESP_OK)
    {
        message = "Wifi connected...";
        dsp->SetContent(0, message);
    }
    else
    {
        message = "Wifi NOT connected...";
        dsp->SetContent(0, message);
        ESP_LOGI(taskName, "Wifi error %d", ret);
    }
    return ret;
    ESP_LOGI(taskName, "Wifi startup end");
}

void mainTask(void *param)
{
    char *taskName = pcTaskGetName(0);
    int wifiStaRetry = 0;
    bool running;
    bool reset = false;
    ringbufferSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(ringbufferSemaphore);
    std::string message("");
    stationQueue = xQueueCreate(5, sizeof(int));
    commandQueue = xQueueCreate(5, sizeof(Vs1053Commands));
    uint32_t notificationValue;
    // Display_init
    Display dsp(oSettings.display.sdapin, oSettings.display.sclpin, oSettings.display.rows, oSettings.display.cols);
    display = &dsp;
    display->AddHUNChars();
    ESP_ERROR_CHECK(display->AddSegment(DisplaySegment(0, Bounds(0, 0, oSettings.display.cols, 0))));
    ESP_ERROR_CHECK(display->AddSegment(DisplaySegment(1, Bounds(1, 0, oSettings.display.cols, 1))));
    message = "Display on..";
    display->SetContent(0, message);
    message = "ORadio is starting...";
    display->SetContent(1, message);

    // first station
    xQueueSend(stationQueue, &lastStation, playList.defaultStation);
    if (!oSettings.wifi.startasap)
    {
        ESP_LOGI(taskName, "Wifi %s - %s", oSettings.wifi.stassid.c_str(), oSettings.wifi.stapassword.c_str());
        wifi.InitAsSTA(oSettings.wifi.stassid.c_str(), oSettings.wifi.stapassword.c_str());
        ESP_ERROR_CHECK(initWifi(display));
        ESP_ERROR_CHECK(wifi.Connect());
    }
    else
    {
        ESP_LOGI(taskName, "Wifi %s - %s", oSettings.wifi.apssid.c_str(), oSettings.wifi.appassword.c_str());
        wifi.InitAsAP(oSettings.wifi.apssid.c_str(), oSettings.wifi.appassword.c_str());
        ESP_ERROR_CHECK(initWifi(display));
        ESP_ERROR_CHECK(wifi.Connect());
        message = "Connect to SSID:" + oSettings.wifi.apssid;
        display->SetContent(0, message);
        message = "Password: " + oSettings.wifi.appassword;
        display->SetContent(1, message);
    }

    if (!oServer.started)
    {
        ESP_LOGI(taskName, "Starting http server");
        oServer.start(changeme, setVolume, reloadPlaylist, restart);
    }

    while (!reset)
    {
        // wait and we will get start or stop
        // if wifi disconnect then the inside loop already set the notofocation so we will rin into the correct case
        if (pdTRUE == xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, 100 / portTICK_PERIOD_MS))
        {
            switch (notificationValue)
            {
            case TASK_ARRAY_GRACEFUL_RESTART:
            {
                reset = true;
            }
            case TASK_ARRAY_WIFI_NOT_CONNECTED:
            {
                wifiStaRetry++;
                ESP_LOGI(taskName, "Restarting main");
                // wifi disconnected
                // stop tasks and set last station into queue
                // stop fetcher and streaming
                xTaskNotify(fetcherTaskHandle, TASK_ARRAY_RESET_NOTIFY, eSetValueWithoutOverwrite);
                // wait for them to signal back
                xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
                xQueueSend(stationQueue, &lastStation, 0);
                ESP_LOGI(taskName, "Wifi not connected retrying...");
                ESP_ERROR_CHECK(wifi.Disconnect());
                vTaskDelay(100 / portTICK_PERIOD_MS);
                if (wifiStaRetry < READ_MAX_RETRY)
                {
                    ESP_ERROR_CHECK(wifi.Connect());
                }
                else
                {
                    // no more try reset sta credentials and restart in AP mode
                    ESP_LOGI(taskName, "STA unable changing back to AP");
                    oSettings.wifi.startasap = true;
                    auto changedSettings = SettingsToString(oSettings);
                    ESP_LOGI(taskName, "%s", changedSettings.c_str());
                    std::string path = FS_MOUNT_POINT;
                    path += "/settings.json";
                    std::ofstream fs(path, std::ofstream::trunc);
                    if (fs.good())
                    {
                        ESP_LOGI(taskName, "Settings file opened for write");
                        OFileSystem::writeToFileStream(changedSettings.c_str(), fs, changedSettings.length());
                    }
                    fs.close();
                    ESP_LOGI(taskName, "Settings file closed");
                    reset = true;
                }
                break;
            }
            case TASK_ARRAY_WIFI_CONNECTED:
            {
                running = true;
                while (running)
                {
                    if (xQueueReceive(stationQueue, &lastStation, 100 / portTICK_PERIOD_MS))
                    {
                        ESP_LOGI(taskName, "Got station %d", lastStation);
                        // clear out multiple changings faster than half a second
                        while (xQueueReceive(stationQueue, &lastStation, 500 / portTICK_PERIOD_MS))
                            ;
                        // stop fetcher and streaming
                        xTaskNotify(fetcherTaskHandle, TASK_ARRAY_RESET_NOTIFY, eSetValueWithoutOverwrite);
                        // wait for them to signal back
                        xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
                        message = "No name";
                        display->SetContent(0, message);
                        message = "Various";
                        display->SetContent(1, message);

                        xTaskNotify(fetcherTaskHandle, TASK_ARRAY_START_NOTFIY, eSetValueWithoutOverwrite);
                    }
                    display->ProcessContents();
                    // at this point we will could get not connected only
                    if (pdPASS == xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, 0))
                    {
                        switch (notificationValue)
                        {
                        case TASK_ARRAY_GRACEFUL_RESTART:
                        {
                            reset = true;
                        }
                        case TASK_ARRAY_WIFI_NOT_CONNECTED:
                        {
                            running = false;
                        }
                        }
                    }

                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
                break;
            }
            }
        }
        else
        {
            display->ProcessContents();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
    }
    ESP_LOGI(taskName, "Shutting down");
    xTaskNotify(fetcherTaskHandle, TASK_ARRAY_GRACEFUL_RESTART, eSetValueWithoutOverwrite);
    xTaskNotifyWait(ULONG_MAX, pdFALSE, &notificationValue, portMAX_DELAY);
    ESP_LOGI(taskName, "Stopping server");
    oServer.stop();
    ESP_LOGI(taskName, "Disconnecting wifi");
    wifi.Disconnect();
    ESP_LOGI(taskName, "RESTARTING...");
    esp_restart();
    vTaskDelete(mainTaskHandle);
}

extern "C" void app_main(void)
{
    char *taskName = pcTaskGetName(0);
    // NVS
    ESP_ERROR_CHECK(OFileSystem::initNvs());
    ESP_ERROR_CHECK(OFileSystem::initFileSystem());

    ESP_LOGI(WIFI_TAG, "Starting wifi driver and event loop");
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    reloadPlaylist();
    reloadSettings();

    // for (int i = 0; i < playList.stationsCount; i++)
    // {
    //     ESP_LOGI(taskName, "%d - %s - %s", playList.stations[i].id, playList.stations[i].url.c_str(), playList.stations[i].desc.c_str());
    // }

    vs1503Device.setup(oSettings.vs1053.xcspin, oSettings.vs1053.xdcspin, oSettings.vs1053.dreqpin, oSettings.vs1053.resetpin, oSettings.vs1053.mosipin, oSettings.vs1053.misopin, oSettings.vs1053.sclkpin);
    vs1503Device.spi_master_init();
    ESP_LOGI(taskName, "spi_master_init done");
    vs1503Device.startSong();
    vs1503Device.switchToMp3Mode();
    vs1503Device.loadDefaultVs1053Patches();
    vs1503Device.streamMode(false);
    ESP_LOGI(taskName, "CONFIG_VOLUME=%d", CONFIG_VOLUME);
    vs1503Device.setVolume(CONFIG_VOLUME);

    ringBuffer = xRingbufferCreate(RING_BUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    assert(ringBuffer);
    // isr for buttons
    gpio_install_isr_service(0);
    setupButton(GPIO_NUM_34, &prevStationButton);
    setupButton(GPIO_NUM_35, &nextStationButton);
    setupButton(GPIO_NUM_32, &volumeDownButton);
    setupButton(GPIO_NUM_33, &volumeUpButton);
    //  main thread
    xTaskCreatePinnedToCore(mainTask, "Main", 8192, NULL, 3, &mainTaskHandle, 0);
    configASSERT(mainTaskHandle);
    // fetcher
    xTaskCreatePinnedToCore(fetchTask, "Fetcher", 4096, NULL, 2, &fetcherTaskHandle, 0);
    configASSERT(fetcherTaskHandle);
    // streaming thread
    xTaskCreatePinnedToCore(streamingTask, "Streaming", 2048, NULL, 2, &streamingTaskHandle, 1);
    configASSERT(streamingTaskHandle);
    ESP_LOGI("app_main", "Oradio started");
    heap_caps_print_heap_info(MALLOC_CAP_INTERNAL);
}
