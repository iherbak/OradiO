#pragma once

#include "esp_vfs.h"

#define FILE_PATH_MAX ESP_VFS_PATH_MAX + 128
#define SCRATCH_BUFSIZE 2048

typedef void (*stationChangeCallback)(int channelId);
typedef void (*volumeSetCallback)(bool up);
typedef void (*reloadPlaylistCallback)();
typedef void (*restartCallback)();

class Context
{
public:
    char scratch[SCRATCH_BUFSIZE];
    stationChangeCallback stationChange;
    volumeSetCallback volumeChange;
    reloadPlaylistCallback reloadPlaylist;
    restartCallback restart;
};
