#ifndef MAIN_VS1053_H_
#define MAIN_VS1053_H_

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string>

// SCI Register
#define VS_WRITE_COMMAND 0x02
#define VS_READ_COMMAND 0x03
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0a
#define SCI_VOL 0x0b
#define SCI_AICTRL0 0x0c
#define SCI_AICTRL1 0x0d
#define SCI_AICTRL2 0x0e
#define SCI_AICTRL3 0x0f
#define SCI_num_registers 0x0f
#define ADDR_REG_GPIO_DDR_RW 0xc017
#define ADDR_REG_GPIO_ODATA_RW 0xc019

// SCI_MODE bits
#define SM_SDINEW 11 // Bitnumber in SCI_MODE always on
#define SM_RESET 2   // Bitnumber in SCI_MODE soft reset
#define SM_CANCEL 3  // Bitnumber in SCI_MODE cancel song
#define SM_TESTS 5   // Bitnumber in SCI_MODE for tests
#define SM_STREAM 6  // Bitnumber in SCI_MODE for stream mode
#define SM_LINE1 14  // Bitnumber in SCI_MODE for Line input

#define LOW 0
#define HIGH 1
#define VS1053_CHUNK_SIZE 32
#define _BV(bit) (1 << (bit))

class VS1053
{

public:
    gpio_num_t xcs_pin;
    gpio_num_t xdcs_pin;
    gpio_num_t dreq_pin;
    gpio_num_t reset_pin;
    gpio_num_t mosi_pin;
    gpio_num_t miso_pin;
    gpio_num_t sclk_pin;

    uint8_t curvol;      // Current volume setting 0..100%
    uint8_t endFillByte; // Byte to send when stopping song
    uint8_t chipVersion; // Version of hardware
    spi_device_handle_t SPIHandleLow;
    spi_device_handle_t SPIHandleFast;

    VS1053();
    void setup(gpio_num_t gpio_XCS, gpio_num_t gpio_XDCS, gpio_num_t gpio_DREQ, gpio_num_t gpio_RESET, gpio_num_t gpio_MOSI, gpio_num_t gpio_MISO, gpio_num_t gpio_SCLK);

    void beginOutput();

    void startSong();                          // Prepare to start playing. Call this each
                                               // time a new song starts.
    void playChunk(uint8_t *data, size_t len); // Play a chunk of data.  Copies the data to
                                               // the chip.  Blocks until complete.
    void stopSong();                           // Finish playing a song. Call this after
                                               // the last playChunk call.
    void setVolume(uint8_t vol);               // Set the player volume.Level from 0-100,
                                               // higher is louder.
    void setTone(uint8_t *rtone);              // Set the player baas/treble, 4 nibbles for
                                               // treble gain/freq and bass gain/freq
    uint8_t getVolume();                       // Get the currenet volume setting.
                                               // higher is louder.
    void printDetails(std::string &header);           // Print configuration details to serial output.
    void softReset();                          // Do a soft reset
    bool testComm(std::string &header);               // Test communication with module
    void switchToMp3Mode();
    bool isChipConnected();
    uint16_t getDecodedTime(); // Provides SCI_DECODE_TIME register value
    void clearDecodedTime();   // Clears SCI_DECODE_TIME register (sets 0x00)
    uint8_t getHardwareVersion();
    uint16_t readHDat0();
    uint16_t readHDat1();
    void changeSampleRate(uint16_t value);
    void writeClock(uint16_t value);
    void streamMode(bool onoff);
    void loadDefaultVs1053Patches();
    void spi_master_init();

private:
    void await_data_request();
    bool current_data_request();
    void control_mode_on();
    void control_mode_off();
    void data_mode_on();
    void data_mode_off();
    uint16_t read_register(uint8_t _reg);
    bool write_register(uint8_t _reg, uint16_t _value);
    bool sdi_send_buffer(uint8_t *data, size_t len);
    bool sdi_send_fillers(size_t length);
    void wram_write(uint16_t address, uint16_t data);
    uint16_t wram_read(uint16_t address);
    void loadUserCode(const unsigned short *plugin, unsigned short plugin_size);
    long map(long x, long in_min, long in_max, long out_min, long out_max);
};

// public
#endif /* MAIN_VS1053_H_ */
