FILE(GLOB_RECURSE sources ./*.cpp ./model/*.cpp ./includes/*.h)

idf_component_register(SRCS ${sources} INCLUDE_DIRS "."
    REQUIRES LCD44780 Wifi StreamReader OServer OFileSystem Vs1053 esp_ringbuf bblanchon__arduinojson)

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../www")

if(EXISTS ${WEB_SRC_DIR}/dist)
    littlefs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'npm run build' in ${WEB_SRC_DIR}")
endif()