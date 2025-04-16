#include "ofileSystem.h"
#include <esp_log.h>
#include "nvs_flash.h"
#include <string>
#include <sys/_default_fcntl.h>
#include <sys/unistd.h>
#include <fstream>
#include <sstream>
#include "esp_littlefs.h"

esp_err_t OFileSystem::initFileSystem()
{

    esp_vfs_littlefs_conf_t conf = {
        .base_path = FS_MOUNT_POINT,
        .partition_label = "www",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    // Use settings defined above to initialize and mount LittleFS filesystem.
    // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(FILESYSTEM_TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(FILESYSTEM_TAG, "Failed to find LittleFs partition");
        }
        else
        {
            ESP_LOGE(FILESYSTEM_TAG, "Failed to initialize LittleFs (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_littlefs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(FILESYSTEM_TAG, "Failed to get LittleFs partition information (%s)", esp_err_to_name(ret));
    }
    else
    {
        ESP_LOGI(FILESYSTEM_TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
};

esp_err_t OFileSystem::initNvs()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

int OFileSystem::readFromFileStream(char* buffer, std::ifstream &fileStream, int read){
    return fileStream.readsome(buffer,read);
}

void OFileSystem::writeToFileStream(const char* buffer, std::ofstream &fileStream, int write){
    fileStream.write(buffer,write);
}

esp_err_t OFileSystem::readFile(const char *filePath, std::string &content)
{
    std::string fullPath(FS_ROOT_FOLDER);
    fullPath += filePath;
    std::ifstream f(fullPath);
    if (f)
    {
        ESP_LOGI(FILESYSTEM_TAG,"Opened %s",fullPath.c_str());
        std::stringstream ss;
        ss << f.rdbuf();
        content = ss.str();
        f.close();
        return ESP_OK;
    }
    return ESP_ERR_NOT_FOUND;
}