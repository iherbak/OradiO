/**
 * Tochiba 47880 LCD connector using I2C interface with chip Pcf8574t
 * The I2C is using 4 pin to connect to the LCD therefore we need to put it in 4bit mode.
 * One byte sent to the I2C mapped as following
 *
 *  7   6   5   4   3   2   1   0
 *  D7  D6  D5  D4  BL  EN  RW  RS
 *
 * D7-D4 data to LCD HIGH bits then low bits
 * BL controls LCD backlight only
 * EN Pulse signal to send to LCD
 * RW - read or write
 * RS - select Command or data register
 */


#pragma once


#include <driver/i2c_master.h>
#include <string>
#define RS_COMMAND 0x00
#define RS_DATA 0x01
#define RW_READ 0x02
#define RW_WRITE 0x00

#define PULSE 0x04
#define BLACKLIGHT 0x08

/* HD4478 commands*/
#define CLEAR_DISPLAY 0x01
#define HOME 0x02

#define ENTRY_MODE_SET 0x04
#define ENTRY_MODE_CURSOR_DIRECTION_INCREMENT 0x02
#define ENTRY_MODE_CURSOR_DIRECTION_DECREMENT 0x00
#define ENTRY_MODE_DISPLAY_SHIFT 0x01
#define ENTRY_MODE_CURSOR_MOVE 0x00

#define DISPLAY_CONTROL 0x08
#define DISPLAY_CONTROL_DISPLAY_ON 0x04
#define DISPLAY_CONTROL_DISPLAY_OFF 0x00
#define DISPLAY_CONTROL_CURSOR_ON 0x02
#define DISPLAY_CONTROL_CURSOR_OFF 0x00
#define DISPLAY_CONTROL_CURSOR_BLINKING_ON 0x01
#define DISPLAY_CONTROL_CURSOR_BLINKING_OFF 0x00

#define CURSOR_SHIFT 0x10
#define CURSOR_SHIFT_DISPLAY_SHIFT 0x08
#define CURSOR_SHIFT_CURSOR_SHIFT 0x00
#define CURSOR_SHIFT_RIGHT 0x04
#define CURSOR_SHIFT_LEFT 0x00

#define FUNCTION_SET 0x20
#define FUNCTION_SET_8BIT_MODE 0x10
#define FUNCTION_SET_4BIT_MODE 0x00
#define FUNCTION_SET_2LINES 0x08
#define FUNCTION_SET_1LINE 0x00
#define FUNCTION_SET_CHARS_5X10 0x04
#define FUNCTION_SET_CHARS_5X8 0x00

#define SET_DDRAM_ADDRESS 0x80
#define SET_CGRAM_ADDRESS 0x40

class Lcd44780
{
public:
    Lcd44780(uint16_t device_address, gpio_num_t sda, gpio_num_t scl, uint32_t master_freq = 10000);
    esp_err_t initIn4bitMode(uint8_t display_rows, uint8_t display_cols);
    esp_err_t write(std::string text);
    esp_err_t write(const char *text);
    esp_err_t clear();
    esp_err_t home();
    esp_err_t moveCursor(uint8_t row, uint8_t col);
    esp_err_t createCustomChar(int index, uint8_t bitmap[8]);
private:
    uint8_t _row_offsets[4] = {0x00, 0x40, 0x00, 0x00};
    uint8_t _display_rows = 2;
    uint8_t _display_cols = 16;
    i2c_master_bus_handle_t bus_handle;
    i2c_master_bus_config_t bus_config;
    i2c_device_config_t dev_config;
    i2c_master_dev_handle_t dev_handle;
    esp_err_t sendModeInit(uint8_t command, int wait);
    esp_err_t sendAsNibbles(uint8_t data, bool iscommand = true);
    esp_err_t sendToI2C(uint8_t *data, uint8_t datalength, uint16_t wait = 0);
    uint8_t waitForLCDBusy();
};
