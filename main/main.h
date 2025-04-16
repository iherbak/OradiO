#pragma once
#include "./models/playlist.h"

Playlist playList;

void extractMeta(int &readSum, int &read, uint8_t &metaLength, uint8_t *buffer, std::string &icymeta, int &readMeta);
void getMetaStreamTitle(std::string &icyMeta);
esp_err_t httpHandler(esp_http_client_event_t *event);