#pragma once

#include <esp_err.h>
#include <string>

#define FILESYSTEM_TAG "FileSystem"
#define FS_MOUNT_POINT "/www"
#define FS_ROOT_FOLDER "/www/"

class OFileSystem
{
public:
    static esp_err_t initFileSystem();
    static esp_err_t initNvs();
    static esp_err_t readFile(const char *filePath, std::string &content);
    static int readFromFileStream(char* buffer, std::ifstream &fileStream, int read);
    static void writeToFileStream(const char* buffer, std::ofstream &fileStream, int write);
};