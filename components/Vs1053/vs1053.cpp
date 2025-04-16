#include "vs1503patches.h"
#include "vs1053b-patches-pitch.h"
#include "VS1053.h"
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

#include "vs1053.h"
#include <string>

#define TAG "VS1053"
#define _DEBUG_ 0

VS1053::VS1053()
{
}

void VS1053::setup(gpio_num_t gpio_XCS, gpio_num_t gpio_XDCS, gpio_num_t gpio_DREQ, gpio_num_t gpio_RESET, gpio_num_t gpio_MOSI, gpio_num_t gpio_MISO, gpio_num_t gpio_SCLK)
{
	dreq_pin = gpio_DREQ;
	xcs_pin = gpio_XCS;
	xdcs_pin = gpio_XDCS;
	reset_pin = gpio_RESET;
	mosi_pin = gpio_MOSI;
	miso_pin = gpio_MISO;
	sclk_pin = gpio_SCLK;
}

void VS1053::beginOutput()
{
	esp_err_t ret;
	ESP_LOGI(TAG, "GPIO_DREQ=%d", dreq_pin);
	gpio_reset_pin(dreq_pin);
	ESP_ERROR_CHECK(gpio_set_direction(xcs_pin, GPIO_MODE_INPUT));
	// ESP_ERROR_CHECK(gpio_ set_pull_mode(dreq_pin, GPIO_PULLUP_DISABLE));
	ESP_ERROR_CHECK(gpio_intr_disable(dreq_pin));

	ESP_LOGI(TAG, "GPIO_CS=%d", xcs_pin);
	ESP_ERROR_CHECK(gpio_reset_pin(xcs_pin));
	ESP_ERROR_CHECK(gpio_set_direction(xcs_pin, GPIO_MODE_OUTPUT));

	ESP_LOGI(TAG, "GPIO_DCS=%d", xdcs_pin);
	ESP_ERROR_CHECK(gpio_reset_pin(xdcs_pin));
	ESP_ERROR_CHECK(gpio_set_direction(xdcs_pin, GPIO_MODE_OUTPUT));
};

void VS1053::spi_master_init()
{
	esp_err_t ret;

	ESP_LOGI(TAG, "GPIO_DREQ=%d", dreq_pin);
	gpio_reset_pin(dreq_pin);
	ESP_ERROR_CHECK(gpio_set_direction(dreq_pin, GPIO_MODE_INPUT));
	ESP_ERROR_CHECK(gpio_pulldown_dis(dreq_pin));
	ESP_ERROR_CHECK(gpio_pullup_dis(dreq_pin));
	ESP_ERROR_CHECK(gpio_intr_disable(dreq_pin));

	ESP_LOGI(TAG, "GPIO_CS=%d", xcs_pin);
	ESP_ERROR_CHECK(gpio_reset_pin(xcs_pin));
	ESP_ERROR_CHECK(gpio_set_direction(xcs_pin, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_intr_disable(xcs_pin));

	ESP_LOGI(TAG, "GPIO_DCS=%d", xdcs_pin);
	ESP_ERROR_CHECK(gpio_reset_pin(xdcs_pin));
	ESP_ERROR_CHECK(gpio_set_direction(xdcs_pin, GPIO_MODE_OUTPUT));
	ESP_ERROR_CHECK(gpio_intr_disable(xdcs_pin));

	// gpio_dump_io_configuration(stdout, (1ULL << 16) | (1ULL << 25) | (1ULL << 27));

	gpio_set_level(xcs_pin, HIGH);
	gpio_set_level(xdcs_pin, HIGH);
	vTaskDelay(100 / portTICK_PERIOD_MS);

	spi_bus_config_t buscfg = {
		.mosi_io_num = mosi_pin,
		.miso_io_num = miso_pin,
		.sclk_io_num = sclk_pin,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.flags = SPICOMMON_BUSFLAG_MASTER};

	ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
	assert(ret == ESP_OK);

	// Init SPI in slow mode
	// ESP_LOGI(TAG,"VS1053 LowFreq: %d",freq);
	int freq = 2 * 1000 * 1000;
	spi_device_interface_config_t devcfg = {
		.command_bits = 8,
		.address_bits = 8,
		.dummy_bits = 0,
		.mode = 0,
		.duty_cycle_pos = 0,
		.cs_ena_pretrans = 0,
		.cs_ena_posttrans = 1,
		.clock_speed_hz = freq,
		.spics_io_num = -1,
		.flags = SPI_DEVICE_NO_DUMMY,
		.queue_size = 1};

	spi_device_handle_t lvsspi;
	ret = spi_bus_add_device(HSPI_HOST, &devcfg, &lvsspi);
	ESP_LOGD(TAG, "spi_bus_add_device=%d", ret);
	assert(ret == ESP_OK);
	vTaskDelay(20 / portTICK_PERIOD_MS);
	SPIHandleLow = lvsspi;
	// Init SPI in high mode
	spi_device_handle_t hvsspi;
	std::string m("Slow SPI,Testing VS1053 read/write registers...\n");
	if (testComm(m))
	{
		// softReset();
		//  Switch on the analog parts
		write_register(SCI_AUDATA, 0xAC45); // 44.1kHz stereo
		// The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
		write_register(SCI_CLOCKF, 0x2000); // Normal clock settings multiplyer 3.0 = 12.2 MHz
		// SPI Clock to 4 MHz. Now you can set high speed SPI clock.
		// VS1053_SPI = SPISettings(4000000, MSBFIRST, SPI_MODE0);

		// freq =spi_cal_clock(APB_CLK_FREQ, 6100000, 128, NULL);
		//  ESP_LOGI(TAG,"VS1053 HighFreq: %d",freq);
		devcfg.clock_speed_hz = 4 * 1000 * 1000;
		devcfg.command_bits = 0;
		devcfg.address_bits = 0;
		ret = spi_bus_add_device(HSPI_HOST, &devcfg, &hvsspi);
		ESP_LOGD(TAG, "spi_bus_add_device=%d", ret);
		assert(ret == ESP_OK);
		write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_LINE1));
		m = "Fast SPI, Testing VS1053 read/write registers again...Takes a little time\n";
		testComm(m);
		ESP_LOGI(TAG, "testComm end");
		vTaskDelay(10 / portTICK_PERIOD_MS);
		await_data_request();
		endFillByte = wram_read(0x1E06) & 0xFF;
		ESP_LOGI(TAG, "endFillByte=%x", endFillByte);
		// printDetails("After last clock setting");
		chipVersion = getHardwareVersion();
		ESP_LOGI(TAG, "chipVersion=%x", chipVersion);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}

	SPIHandleFast = hvsspi;
};

void VS1053::await_data_request()
{
	while (gpio_get_level(dreq_pin) != HIGH)
	{
		taskYIELD();
	}
};

bool VS1053::current_data_request()
{
	return (gpio_get_level(dreq_pin) == HIGH);
};

void VS1053::control_mode_on()
{
	gpio_set_level(xdcs_pin, HIGH); // Bring slave in control mode
	gpio_set_level(xcs_pin, LOW);
};

void VS1053::control_mode_off()
{
	gpio_set_level(xcs_pin, HIGH); // End control mode
};

void VS1053::data_mode_on()
{
	gpio_set_level(xcs_pin, HIGH); // Bring slave in data mode
	gpio_set_level(xdcs_pin, LOW);
};

void VS1053::data_mode_off()
{
	gpio_set_level(xdcs_pin, HIGH); // End data mode
};

uint16_t VS1053::read_register(uint8_t _reg)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	await_data_request(); // Wait for DREQ to be HIGH
	control_mode_on();
	memset(&SPITransaction, 0, sizeof(spi_transaction_t));
	SPITransaction.length = 16;
	SPITransaction.flags |= SPI_TRANS_USE_RXDATA;
	SPITransaction.cmd = VS_READ_COMMAND;
	SPITransaction.addr = _reg;

	ret = spi_device_transmit(SPIHandleLow, &SPITransaction);
	assert(ret == ESP_OK);
	uint16_t result = (((SPITransaction.rx_data[0] & 0xFF) << 8) | ((SPITransaction.rx_data[1]) & 0xFF));
	control_mode_off();
	return result;
};

bool VS1053::write_register(uint8_t _reg, uint16_t _value)
{
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	await_data_request(); // Wait for DREQ to be HIGH
	control_mode_on();
	memset(&SPITransaction, 0, sizeof(spi_transaction_t));
	SPITransaction.flags |= SPI_TRANS_USE_TXDATA;
	SPITransaction.cmd = VS_WRITE_COMMAND;
	SPITransaction.addr = _reg;
	SPITransaction.tx_data[0] = (_value >> 8) & 0xFF;
	SPITransaction.tx_data[1] = (_value & 0xFF);
	SPITransaction.length = 16;
	ret = spi_device_transmit(SPIHandleLow, &SPITransaction);
	assert(ret == ESP_OK);
	control_mode_off();
	return true;
};

bool VS1053::sdi_send_buffer(uint8_t *data, size_t len)
{
	size_t chunk_length; // Length of chunk 32 byte or shorter
	spi_transaction_t SPITransaction;
	esp_err_t ret;

	while (len) // More to do?
	{
		data_mode_on();
		await_data_request(); // Wait for space available
		chunk_length = len;
		if (len > VS1053_CHUNK_SIZE)
		{
			chunk_length = VS1053_CHUNK_SIZE;
		}
		len -= chunk_length;
		memset(&SPITransaction, 0, sizeof(spi_transaction_t));
		SPITransaction.length = chunk_length * 8;
		SPITransaction.tx_buffer = data;
		ret = spi_device_transmit(SPIHandleFast, &SPITransaction);
		assert(ret == ESP_OK);
		data += chunk_length;
		data_mode_off();
	}
	return true;
};

bool VS1053::sdi_send_fillers(size_t len)
{
	size_t chunk_length; // Length of chunk 32 byte or shorter
	spi_transaction_t SPITransaction;
	esp_err_t ret;
	uint8_t data[VS1053_CHUNK_SIZE];
	for (int i = 0; i < VS1053_CHUNK_SIZE; i++)
		data[i] = endFillByte;

	while (len) // More to do?
	{
		await_data_request(); // Wait for space available
		data_mode_on();
		chunk_length = len;
		if (len > VS1053_CHUNK_SIZE)
		{
			chunk_length = VS1053_CHUNK_SIZE;
		}
		len -= chunk_length;

		memset(&SPITransaction, 0, sizeof(spi_transaction_t));
		SPITransaction.length = chunk_length * 8;
		SPITransaction.tx_buffer = data;
		ret = spi_device_transmit(SPIHandleFast, &SPITransaction);
		assert(ret == ESP_OK);
		data_mode_off();
	}
	return true;
}

void VS1053::wram_write(uint16_t address, uint16_t data)
{
	write_register(SCI_WRAMADDR, address);
	write_register(SCI_WRAM, data);
}

uint16_t VS1053::wram_read(uint16_t address)
{
	write_register(SCI_WRAMADDR, address); // Start reading from WRAM
	return read_register(SCI_WRAM);		   // Read back result
}

void VS1053::changeSampleRate(uint16_t value)
{
	write_register(SCI_AUDATA, value);
}

void VS1053::writeClock(uint16_t value)
{
	write_register(SCI_CLOCKF, value);
}

uint16_t VS1053::readHDat0()
{
	return read_register(SCI_HDAT0);
}

uint16_t VS1053::readHDat1()
{
	return read_register(SCI_HDAT1);
}

bool VS1053::testComm(std::string &header)
{
	int i; // Loop control
	uint16_t r1, r2, cnt = 0;
	uint16_t delta = 300; // 3 for fast SPI

	if (!gpio_get_level(dreq_pin))
	{
		ESP_LOGW(TAG, "VS1053 not properly installed!");
		// Allow testing without the VS1053 module
		// pinMode(dreq_pin, INPUT_PULLUP); // DREQ is now input with pull-up
		return false; // Return bad result
	}
	// Further TESTING.  Check if SCI bus can write and read without errors.
	// We will use the volume setting for this.
	// Will give warnings on serial output if DEBUG is active.
	// A maximum of 20 errors will be reported.
	if (header == "Fast")
	{
		delta = 3; // Fast SPI, more loops
	}

	ESP_LOGI(TAG, "%s", header.c_str()); // Show a header

	for (i = 0; (i < 0xFFFF) && (cnt < 20); i += delta)
	{
		write_register(SCI_VOL, i);			// Write data to SCI_VOL
		r1 = read_register(SCI_VOL);		// Read back for the first time
		r2 = read_register(SCI_VOL);		// Read back a second time
		if (r1 != r2 || i != r1 || i != r2) // Check for 2 equal reads
		{
			ESP_LOGW(TAG, "VS1053 error retry SB:%04X R1:%04X R2:%04X", i, r1, r2);
			cnt++;
			vTaskDelay(10 / portTICK_PERIOD_MS);
		}
		taskYIELD(); // Allow ESP firmware to do some bookkeeping
	}
	return (cnt == 0); // Return the result
}

long VS1053::map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void VS1053::setVolume(uint8_t vol)
{
	uint16_t value; // Value to send to SCI_VOL

	curvol = vol;						  // Save for later use
	value = map(vol, 0, 100, 0xFF, 0x00); // 0..100% to one channel
	value = (value << 8) | value;
	write_register(SCI_VOL, value); // Volume left and right
}

void VS1053::setTone(uint8_t *rtone)
{
	uint16_t value = 0; // Value to send to SCI_BASS
	int i;				// Loop control

	for (i = 0; i < 4; i++)
	{
		value = (value << 4) | rtone[i]; // Shift next nibble in
	}
	write_register(SCI_BASS, value); // Volume left and right
}

uint8_t VS1053::getVolume()
{ // Get the currenet volume setting.
	return curvol;
}

void VS1053::startSong()
{
	sdi_send_fillers(10);
}

void VS1053::playChunk(uint8_t *data, size_t len)
{
	sdi_send_buffer(data, len);
}

void VS1053::stopSong()
{
	uint16_t modereg; // Read from mode register
	int i;			  // Loop control
	write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_CANCEL));
	for (i = 0; i < 200; i++)
	{
		sdi_send_fillers(32);
		modereg = read_register(SCI_MODE); // Read status
		if ((modereg & _BV(SM_CANCEL)) == 0)
		{
			endFillByte = wram_read(0x1E06) & 0xFF;
			sdi_send_fillers(2052);
			ESP_LOGI(TAG, "Song stopped correctly after %d msec", i * 10);
			return;
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);
	}
	std::string a("Song stopped incorrectly!");
	printDetails(a);
}

void VS1053::softReset()
{
	ESP_LOGI(TAG, "Performing soft-reset");
	write_register(SCI_MODE, _BV(SM_SDINEW) | _BV(SM_RESET));
	vTaskDelay(10 / portTICK_PERIOD_MS);
	await_data_request();
}

void VS1053::printDetails(std::string &header)
{
	ESP_LOGI(TAG, "%s", header.c_str());
	ESP_LOGI(TAG, "REG	 Contents");
	ESP_LOGI(TAG, "---	 -----");
	for (int i = 0; i <= SCI_num_registers; i++)
	{
		uint16_t regbuf = read_register(i);
		vTaskDelay(5 / portTICK_PERIOD_MS);
		ESP_LOGI(TAG, "%x - %x", i, regbuf);
	}
}

void VS1053::switchToMp3Mode()
{
	wram_write(ADDR_REG_GPIO_DDR_RW, 3);   // GPIO DDR = 3
	wram_write(ADDR_REG_GPIO_ODATA_RW, 0); // GPIO ODATA = 0
	vTaskDelay(100 / portTICK_PERIOD_MS);
	ESP_LOGI(TAG, "Switched to mp3 mode");
	softReset();
}

bool VS1053::isChipConnected()
{
	uint16_t status = read_register(SCI_STATUS);

	return !(status == 0 || status == 0xFFFF);
}

uint16_t VS1053::getDecodedTime()
{
	return read_register(SCI_DECODE_TIME);
}

void VS1053::clearDecodedTime()
{
	// should write twice to prevent firmware overrite the real data
	write_register(SCI_DECODE_TIME, 0x00);
	write_register(SCI_DECODE_TIME, 0x00);
}

uint8_t VS1053::getHardwareVersion()
{
	uint16_t status = read_register(SCI_STATUS);

	return (status >> 4) & 0xf;
}

void VS1053::streamMode(bool onoff) // Set stream mode on/off
{
	uint16_t pat = _BV(SM_SDINEW); // Assume off

	if (onoff) // Activate stream mode?
	{
		pat |= _BV(SM_STREAM); // Yes, set stream mode bit
	}
	write_register(SCI_MODE, pat); // Set new value
	vTaskDelay(10 / portTICK_PERIOD_MS);
	await_data_request();
};

void VS1053::loadUserCode(const unsigned short *plugin, unsigned short plugin_size)
{
	int i = 0;
	while (i < plugin_size / sizeof(plugin[0]))
	{
		unsigned short addr, n, val;
		addr = plugin[i++];
		n = plugin[i++];
		if (n & 0x8000U)
		{ /* RLE run, replicate n samples */
			n &= 0x7FFF;
			val = plugin[i++];
			while (n--)
			{
				write_register(addr, val);
			}
		}
		else
		{ /* Copy run, copy n samples */
			while (n--)
			{
				val = plugin[i++];
				write_register(addr, val);
			}
		}
	}
};

void VS1053::loadDefaultVs1053Patches()
{
	ESP_LOGI("VS1053", "loading user code");
	loadUserCode(PATCHES, PATCHES_SIZE);
	ESP_LOGI("VS1053", "loading user code done");
};