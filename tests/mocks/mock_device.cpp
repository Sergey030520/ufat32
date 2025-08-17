#include "mock_device.hpp"
#include <string.h>
#include <fstream>

static uint8_t mock_storage[MOCK_TOTAL_SECTORS][MOCK_SECTOR_SIZE];
static bool initialized = false;
static char *pathToSD = "";

void mock_device_init()
{
    memset(mock_storage, 0x00, sizeof(mock_storage));
    initialized = true;
}

void mock_device_deinit()
{
    initialized = false;
}

int read_sector(uint8_t *buffer, uint32_t size, uint32_t sector)
{
    std::ifstream file(pathToSD, std::ios_base::binary);
    if (file.is_open() == true)
    {
        puts("Isn't open file!\n");
        return -1;
    }
    file.seekg(sector * 512, std::ios::beg);
    file.read((char *)buffer, size);
    int length = file.gcount();
    file.close();
    return length;
}

int write_sector(const uint8_t *buffer, uint32_t size, uint32_t sector)
{
    std::ofstream file(pathToSD, std::ios_base::binary | std::ios_base::app);
    if (file.is_open() == false)
    {
        puts("Isn't open file!\n");
        return -1;
    }
    file.seekp(sector * 512, std::ios::beg);
    file.write((char *)buffer, size);
    if (!file)
    {
        printf("Failed to write data!\n");
        return -4;
    }
    file.close();
    return size;
}

int clear_device(uint32_t sector_start, uint32_t sector_stop)
{
    std::ofstream file(pathToSD, std::ios_base::binary | std::ios_base::app);
    if (file.is_open() == false)
    {
        puts("Isn't open file!\n");
        return -1;
    }

    const uint8_t buffer[512] = {0};

    file.seekp(sector_start * 512, std::ios::beg);
    if (!file)
    {
        puts("Seek error!\n");
        return -3;
    }


    for (uint32_t idx = sector_start; idx < sector_stop; ++idx)
    {
        file.write((char *)buffer, 512);
        if(!file){
            printf("Error clear data!\n");
        }
        file.flush();
    }
    printf("\n"); // Переход на новую строку после завершения

    file.close();
}
