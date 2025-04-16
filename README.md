| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 | Linux |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- | -------- | ----- |

# ORadio


## Build

Build with VsCode using ESP-IDF extension.
Used SDK version is ES-IDF 5.4
You might need if not yet installed:
 - cmake
 - python

## Build web
In the folder www you find the React based web app
In that folder run
- npm install (you might need to install nodeJS if not yet there)
- after that run npm run build any time you made changes to the web app, then build Oradio and flash it or just flash the www partition
It creates it's output into the dist folder that will be the content of the www partition

## Flash

Flash it using ESP-IDF toolbar flash icon after setting UART and your COM port

## Usage

ORadiO should start in AP mode. You should connect to it's SSID and then open 192.168.4.1. 
Under settings tab on the web interface change the Mode to STA and fill in the SSID and password, then save and restart with the buttons.
It should connect to your wifi and start with the default station.

On the web (currently Hungarian only) you can:
- Change stations, and volume
- edit stations
- edit settings

## BOM

- One ESP32
- LCD 44780 or compatible (I used a very common LCD1602)
- A VS1053B decoder
- 4 button (separate wiring)
- (Optional breakout board for the ESP32)
