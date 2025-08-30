add_library(fat32_lib
    FAT32.c
    fat32_time.c
    file_utils.c
    debug.c
    fat32_alloc.c
    log_fat32.c
)

target_include_directories(fat32_lib PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)


