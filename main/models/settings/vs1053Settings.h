#pragma once
#include <cstdint>

class Vs1053Settings
{
public:
    gpio_num_t dreqpin;
    gpio_num_t xcspin;
    gpio_num_t xdcspin;
    gpio_num_t resetpin;

    gpio_num_t mosipin;
    gpio_num_t misopin;
    gpio_num_t sclkpin;
    uint8_t startvolume;

    Vs1053Settings(){}
    Vs1053Settings(uint8_t dreq, uint8_t xcs, uint8_t xdcs, uint8_t reset,uint8_t mosi, uint8_t miso, uint8_t sclk, uint8_t volume)
    {
        dreqpin = (gpio_num_t)dreq;
        xcspin = (gpio_num_t)xcs;
        xdcspin = (gpio_num_t)xdcs;
        resetpin = (gpio_num_t)reset;
        mosipin =(gpio_num_t)mosi;
        misopin =(gpio_num_t)miso;
        sclkpin =(gpio_num_t)sclk;
        startvolume = volume;
    }
};
