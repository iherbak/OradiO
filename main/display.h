#pragma once

#include "../components/LCD44780/lcd44780.h"
#include "./models/displaySegment.h"
#include <list>
#include <map>
#define DISPLAY_TAG "Display"
class Display
{
public:
    Display(gpio_num_t sda, gpio_num_t scl, uint8_t rows, uint8_t cols);
    esp_err_t AddSegment(DisplaySegment segment);
    DisplaySegment GetSegment(int segmentId);
    esp_err_t SetContent(int targetSegment, std::string &message);
    esp_err_t ProcessContents();
    esp_err_t CreatCustomChar(int index, uint8_t bitmap[8]);
    esp_err_t AddHUNChars();
private:
    esp_err_t layoutCheck(DisplaySegment segment);
    std::map<int,DisplaySegment> _layout;
    QueueHandle_t _displayQueue;
    Lcd44780* _lcd;
    int _activelayer = 0;
    int _nonDefaultLayerTTL = 0;
};
