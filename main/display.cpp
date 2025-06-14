#include "freertos/FreeRTOS.h"
#include <string.h>
#include <portmacro.h>
#include <vector>
#include "display.h"
#include "esp_mac.h"
#include <streambuf>
#include <esp_log.h>
#include "helper.h"
#include <regex>

esp_err_t Display::layoutCheck(DisplaySegment segment)
{
    auto bounds = segment.bounds;
    for (auto itr = _layout.begin(); itr != _layout.end(); itr++)
    {
        // only check collosion between same layer segments
        if (itr->second.layerNumber == segment.layerNumber)
        {
            auto usedBounds = itr->second.bounds;

            // rowcheck
            if ((usedBounds.startRow <= bounds.startRow) && (usedBounds.endRow >= bounds.startRow))
            {
                // column start check
                if ((usedBounds.startColumn <= bounds.startColumn) && (usedBounds.endColumn >= bounds.startColumn))
                {
                    printf("here 1");
                    return ESP_ERR_INVALID_SIZE;
                }
                // column end check
                if ((usedBounds.startColumn <= bounds.endColumn) && (usedBounds.endColumn >= bounds.endColumn))
                {
                    printf("here 2");
                    return ESP_ERR_INVALID_SIZE;
                }
            }
            // rowcheck
            if ((usedBounds.startRow <= bounds.endRow) && (usedBounds.endRow >= bounds.endRow))
            {
                // columncheck
                if ((usedBounds.startColumn <= bounds.startColumn) && (usedBounds.endColumn >= bounds.startColumn))
                {
                    printf("here 3");
                    return ESP_ERR_INVALID_SIZE;
                }

                if ((usedBounds.startColumn <= bounds.endColumn) && (usedBounds.endColumn >= bounds.endColumn))
                {
                    printf("here 4");
                    return ESP_ERR_INVALID_SIZE;
                }
            }
            // bigger check
            if ((usedBounds.startRow >= bounds.startRow) && (usedBounds.endRow <= bounds.endRow) && (usedBounds.startColumn >= bounds.startColumn) && (usedBounds.endColumn >= bounds.endColumn))
            {
                printf("here 5");
                return ESP_ERR_INVALID_SIZE;
            }
        }
    }
    return ESP_OK;
};

Display::Display(gpio_num_t sda, gpio_num_t scl, uint8_t rows, uint8_t cols)
{
    _lcd = new Lcd44780(0x27, sda, scl);
    ESP_ERROR_CHECK(_lcd->initIn4bitMode(rows, cols));
    _displayQueue = xQueueCreate(10, sizeof(DisplayMessage));
    _activelayer = 0;
};

esp_err_t Display::AddSegment(DisplaySegment segment)
{
    if (layoutCheck(segment) == ESP_OK)
    {
        ESP_LOGI("Segment", "layout is ok");
        if (!_layout.contains(segment.id))
        {
            _layout[segment.id] = segment;
            return ESP_OK;
        }
        else
        {
            ESP_LOGE("Segment", "Segment with same ID already exists skipping");
        }
    }
    return ESP_OK;
};

DisplaySegment Display::GetSegment(int segmentId)
{
    return _layout[segmentId];
}

esp_err_t Display::ProcessContents()
{
    std::string s("");
    DisplayMessage message(0, s);
    if (xQueueReceive(_displayQueue, &message, 100 / portTICK_PERIOD_MS))
    {
        // ESP_LOGD("Display", "Looking for segment %d", message.targetSegment);
        auto targetSegment = _layout.find(message.targetSegment);
        Bounds b = targetSegment->second.bounds;
        if (targetSegment->second.layerNumber != _activelayer)
        {
            _activelayer = targetSegment->second.layerNumber;
            _nonDefaultLayerTTL = 10;
        }
        // ESP_LOGI("Display", "Segment found for message %s", message.content);
        std::string content(convertUsingCustomChars(message.content, false));
        if (targetSegment->second.GetContent() != content)
        {
            targetSegment->second.SetContent(content);
            if (!targetSegment->second.scrollData.isScrolling)
            {
                ESP_LOGD("Display", "Text is short, setting segment %d to non scrolling", message.targetSegment);
                // write to screen only if segments layer is active
                if (targetSegment->second.layerNumber == _activelayer)
                {
                    _lcd->moveCursor(b.startRow, b.startColumn);
                    _lcd->write(targetSegment->second.GetContent());
                }
            }
        }
    }

    for (auto itr = _layout.begin(); itr != _layout.end(); itr++)
    {
        if (itr->second.scrollData.isScrolling)
        {
            Bounds b = itr->second.bounds;
            std::string content = itr->second.GetContent();
            // ESP_LOGI("Display", "Processing scrolling element advancing to %d", itr->second.scrollData.scrollPosition);
            if (itr->second.scrollData.scrollPosition + 1 <= (content.length() - (b.endColumn - b.startColumn)))
            {
                itr->second.scrollData.scrollPosition++;
            }
            else
            {
                itr->second.scrollData.scrollPosition = 0;
            }
            // write to screen only if segments layer is active
            if (itr->second.layerNumber == _activelayer)
            {
                _lcd->moveCursor(b.startRow, b.startColumn);
                std::string toWrite = content.substr(itr->second.scrollData.scrollPosition, (b.endColumn - b.startColumn));
                // ESP_LOGD(DISPLAY_TAG, "Will write %s to %d,%d", toWrite.c_str(), b.startRow, b.startColumn);
                _lcd->write(toWrite);
            }
        }
    }
    // countdown on temporary layer
    if (_nonDefaultLayerTTL > 0)
    {
        ESP_LOGE("Display", "DEcresing ttl to %d", _nonDefaultLayerTTL);
        _nonDefaultLayerTTL--;
    }
    // switch back to default layer when times up
    if (_activelayer != 0 && _nonDefaultLayerTTL == 0)
    {
        ESP_LOGE("Display", "Reverting to layer 0");
        _activelayer = 0;
    }

    return ESP_OK;
};

void trimMessage(std::string &message)
{
    std::regex pattern("\\s+");
    message = std::regex_replace(message, pattern, " ");

    // Remove leading and trailing spaces
    size_t firstNonSpace = message.find_first_not_of(" ");
    size_t lastNonSpace = message.find_last_not_of(" ");

    if (firstNonSpace != std::string::npos && lastNonSpace != std::string::npos)
    {
        message = message.substr(firstNonSpace, lastNonSpace - firstNonSpace + 1);
    }
};

esp_err_t Display::SetContent(int targetSegment, std::string &message)
{
    trimMessage(message);
    DisplayMessage dspm(targetSegment, message);
    xQueueSend(_displayQueue, &dspm, 500 / portTICK_PERIOD_MS);
    return ESP_OK;
};

esp_err_t Display::CreatCustomChar(int index, uint8_t bitmap[8])
{
    return _lcd->createCustomChar(index, bitmap);
};

esp_err_t Display::AddHUNChars()
{

    uint8_t smallA[8] = {
        0b00010,
        0b00100,
        0b01110,
        0b00001,
        0b01111,
        0b10001,
        0b01111,
        0b00000}; // á

    uint8_t smallE[8] = {
        0b00010,
        0b00100,
        0b01110,
        0b10001,
        0b11111,
        0b10000,
        0b01110,
        0b00000}; // é

    uint8_t smallI[8] = {
        0b00010,
        0b00100,
        0b00100,
        0b01100,
        0b00100,
        0b00100,
        0b01110,
        0b00000}; // í

    uint8_t smallO[8] = {
        0b00000,
        0b00010,
        0b00100,
        0b01110,
        0b10001,
        0b10001,
        0b01110,
        0b00000}; // ó

    uint8_t smallOO[8] = {
        0b01010,
        0b00000,
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b01110,
        0b00000}; // Ö

    uint8_t smallU[8] = {
        0b00000,
        0b00010,
        0b00100,
        0b10001,
        0b10001,
        0b10011,
        0b01101,
        0b00000}; // ú

    uint8_t smallUU[8] = {
        0b00000,
        0b01010,
        0b00000,
        0b10001,
        0b10001,
        0b10011,
        0b01101,
        0b00000}; // ü

    ESP_ERROR_CHECK(CreatCustomChar(SMALL_A, smallA));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_E, smallE));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_I, smallI));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_O, smallO));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_OO, smallOO));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_U, smallU));
    ESP_ERROR_CHECK(CreatCustomChar(SMALL_UU, smallUU));
    return ESP_OK;
}