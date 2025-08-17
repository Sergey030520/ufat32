add_library(fat32_lib
    DateTime.c
    FAT.c
    file_utils.c
    pool_memory.c
)


target_include_directories(fat32_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
