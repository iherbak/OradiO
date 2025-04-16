#pragma once
#include <cstdint>

class DisplaySettings
{
public:
    gpio_num_t sdapin;
    gpio_num_t sclpin;
    uint8_t rows;
    uint8_t cols;
    DisplaySettings(){}
    DisplaySettings(uint8_t sda, uint8_t scl, uint8_t r, uint8_t c)
    {
        sdapin = (gpio_num_t)sda;
        sclpin = (gpio_num_t)scl;
        rows = r;
        cols = c;
    }
};
