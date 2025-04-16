# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE sources ./*.cpp)

idf_component_register(SRCS ${sources} REQUIRES spiffs nvs_flash joltwallet__littlefs)
