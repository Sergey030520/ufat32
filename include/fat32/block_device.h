#pragma once
#include <stdint.h>
#include "fat32/fat32_types.h"



typedef int (*fs_read_t)(uint8_t *buffer, uint32_t size, uint32_t start_sector, uint32_t sector_size);
typedef int (*fs_write_t)(const uint8_t *buffer, uint32_t size, uint32_t start_sector, uint32_t sector_size);
typedef int (*fs_clear_t)(uint32_t sector_num, uint32_t count_sector, uint32_t sector_size);
typedef int (*fs_get_datetime_t)(Fat32_DateTime *dt);


typedef struct
{
    fs_read_t read;
    fs_write_t write;
    fs_clear_t clear;
    fs_get_datetime_t datetime;
    uint32_t block_size;
} BlockDevice;
