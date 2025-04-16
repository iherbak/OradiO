# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE sources ./*.cpp)

idf_component_register(SRCS ${sources} REQUIRES esp_http_server lwip vfs spiffs nvs_flash esp_netif OFileSystem)
