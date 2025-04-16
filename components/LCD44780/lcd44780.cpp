#include "lcd44780.h"
#include <esp_log.h>
#include <rom/ets_sys.h>
#include <string>
#include <string.h>

/// @brief Toshiba 44780 LCD lib
/// @param device_address
/// @param sda
/// @param scl
/// @param master_freq
Lcd44780::Lcd44780(uint16_t device_address, gpio_num_t sda, gpio_num_t scl, uint32_t master_freq)
{
    bus_handle = nullptr;
    dev_handle = nullptr;

    ESP_LOGD("i2C init", "Initing");
    bus_config = {
        .i2c_port = I2C_MASTER_ACK,
        .sda_io_num = sda, // GPIO_NUM_21,
        .scl_io_num = scl, // GPIO_NUM_22,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags = {.enable_internal_pullup = 1}};

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = master_freq,
        .scl_wait_us = 1000
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    ESP_ERROR_CHECK(i2c_master_probe(bus_handle, device_address, -1));
};

esp_err_t Lcd44780::initIn4bitMode(uint8_t display_rows, uint8_t display_cols)
{
    _display_rows = display_rows;
    _display_cols = display_cols;
    _row_offsets[2] = 0x00 + display_cols;
    _row_offsets[3] = 0x40 + display_cols;

    //ESP_LOGD("i2C init", "Set funcs mode");
    // init sequence 3 times to 8 bit mode and then desired mode
    uint8_t bit8mode = 0 | FUNCTION_SET | FUNCTION_SET_8BIT_MODE;
    uint8_t bit4mode = 0 | FUNCTION_SET | FUNCTION_SET_4BIT_MODE;

    uint8_t cmd_buf[4] = {bit8mode, bit8mode, bit8mode, bit4mode};
    // waits based on 44780 docs
    int waits[4] = {4500, 200, 80, 0};
    esp_err_t ret;
    for (int i = 0; i < 4; i++)
    {
        ret = sendModeInit(cmd_buf[i], waits[i]);
        //ESP_LOGD("i2C init", "ESP_resp is %d", ret);
    }
    bit4mode = 0 | FUNCTION_SET | FUNCTION_SET_2LINES | FUNCTION_SET_CHARS_5X8;
    sendAsNibbles(bit4mode);
    uint8_t command = 0 | DISPLAY_CONTROL | DISPLAY_CONTROL_DISPLAY_ON | DISPLAY_CONTROL_CURSOR_OFF | DISPLAY_CONTROL_CURSOR_BLINKING_OFF;
    sendAsNibbles(command);
    command = 0 | CLEAR_DISPLAY;
    sendAsNibbles(command);
    command = 0 | ENTRY_MODE_SET | ENTRY_MODE_CURSOR_DIRECTION_INCREMENT | ENTRY_MODE_CURSOR_MOVE;
    sendAsNibbles(command);

    return ret;
};

esp_err_t Lcd44780::clear()
{
    uint8_t command = 0 | CLEAR_DISPLAY;
    return sendAsNibbles(command);
};

esp_err_t Lcd44780::home()
{
    uint8_t command = 0 | HOME;
    return sendAsNibbles(command);
};

esp_err_t Lcd44780::moveCursor(uint8_t row, uint8_t col)
{
    uint8_t command = 0 | SET_DDRAM_ADDRESS | (_row_offsets[row] + col);
    return sendAsNibbles(command);
};

esp_err_t Lcd44780::write(const char *text)
{
    std::string s(text);
    return write(s);
};

esp_err_t Lcd44780::write(std::string text)
{
    for (uint8_t c : text)
    {
        sendAsNibbles(c, false);
    }
    return ESP_OK;
};

// Private functions

uint8_t Lcd44780::waitForLCDBusy()
{
    uint8_t readhigh = 0xF0 | BLACKLIGHT | RW_READ;
    uint8_t readlow = 0xF0 | BLACKLIGHT | PULSE | RW_READ;
    uint8_t d[2] = {readhigh, readlow};
    uint8_t read[1] = {0x0};
    // reading high nibble
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, d, sizeof(d), read, sizeof(read), -1));
    // ESP_LOGI("BF read", "Read high is0x%x", read[0] >> 4);
    auto result = read[0] & 0xF0;

    // reading low nibble
    ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, d, sizeof(d), read, sizeof(read), -1));
    // ESP_LOGI("BF read", "Read low is 0x%x", read[0] >> 4);
    result |= (read[0] >> 4);
    // BF flag is the most significant bit
    return (result >> 7);
};

esp_err_t Lcd44780::sendModeInit(uint8_t command, int wait)
{
    //ESP_LOGD("Sending", "0x%x", command);
    uint8_t moddeden = command | BLACKLIGHT | PULSE;
    uint8_t modded = 0x0 | BLACKLIGHT;
    uint8_t d[2] = {moddeden, modded};
    //ESP_LOGD("sendin over", "0x%x 0x%x", moddeden, modded);
    return sendToI2C(d, 2, wait);
};

esp_err_t Lcd44780::sendAsNibbles(uint8_t command, bool isCommand)
{
    ESP_LOGD("Sending", "0x%x", command);
    while (waitForLCDBusy() == 1)
    {
        //ESP_LOGI("Sending", "Display not ready sleep 40 us");
        ets_delay_us(40);
    }
    // high with pulse
    uint8_t nibbles[2] = {0x0, 0x0};
    uint8_t nibble = command & 0xF0;
    nibbles[0] = isCommand ? nibble | BLACKLIGHT | PULSE : nibble | BLACKLIGHT | PULSE | RS_DATA;
    nibbles[1] = isCommand ? nibble | BLACKLIGHT : nibble | BLACKLIGHT | RS_DATA;
    //ESP_LOGD("sendin over", "0x%x 0x%x", nibbles[0], nibbles[1]);
    sendToI2C(nibbles, 2);
    // low with pulse;
    nibble = (command & 0x0F) << 4;
    nibbles[0] = isCommand ? nibble | BLACKLIGHT | PULSE : nibble | BLACKLIGHT | PULSE | RS_DATA;
    nibbles[1] = isCommand ? nibble | BLACKLIGHT : nibble | BLACKLIGHT | RS_DATA;
    //ESP_LOGD("sendin over", "0x%x 0x%x", nibbles[0], nibbles[1]);
    return sendToI2C(nibbles, 2);
};

esp_err_t Lcd44780::sendToI2C(uint8_t *data, uint8_t datalength, uint16_t wait)
{
    esp_err_t ret = i2c_master_transmit(dev_handle, data, datalength, -1);
    return ret;
};

esp_err_t Lcd44780::createCustomChar(int index, uint8_t bitmap[8])
{
    index &= 0x7; // we only have 8 locations 0-7
    uint8_t command = 0 | SET_CGRAM_ADDRESS | (index << 3);
    sendAsNibbles(command);
    for (int i = 0; i < 8; i++)
    {
        sendAsNibbles(bitmap[i], false);
    }
    return ESP_OK;
};