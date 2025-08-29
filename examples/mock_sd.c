#include "mock_sd.h"
#include <stdlib.h>
#include <stdio.h>

char *pathToSD = "/home/sergeyathlete/Code/Boards/repka-pi/ufat32/examples/sd.bin";


int mock_sd_write(const uint8_t *data, uint32_t size, uint32_t address, uint32_t timeout)
{
    FILE *file = fopen(pathToSD, "r+b");
    if (file == NULL)
    {
        puts("Isn't open file!\n");
        return -1;
    }
    fseek(file, address * 512, SEEK_SET);
    fwrite(data, 1, size*512, file);
    fclose(file);
    return 0;
}

int mock_sd_read(uint8_t *buffer, uint32_t size, uint32_t address, uint32_t timeout)
{
    FILE *file = fopen(pathToSD, "r+b");
    if (file == NULL)
    {
        puts("Isn't open file!\n");
        return -1;
    }
    fseek(file, address * 512, SEEK_SET);
    fread(buffer, 1, size*512, file);
    fclose(file);
    return 0;
}


#define UPDATE_INTERVAL_SECTORS 500
#define BAR_WIDTH 50
int mock_sd_erase(uint32_t address_start, uint32_t address_stop)
{
    
    FILE *file = fopen(pathToSD, "r+b");
    if (file == NULL)
    {
        puts("Isn't open file!\n");
        return -1;
    }

    uint8_t buffer[512] = {0};

    if (fseek(file, address_start * 512, SEEK_SET) != 0)
    {
        puts("Seek error!\n");
        fclose(file);
        return -1;
    }
    float progress = 0;
    
    for (uint32_t idx = address_start; idx < address_stop; ++idx)
    {
        fwrite(&buffer[0], 1, 512, file);
        fflush(file);

        if (idx % UPDATE_INTERVAL_SECTORS == 0)
        {
            progress = (float)(idx + 1) / address_stop;
            int pos = progress * BAR_WIDTH;
            printf("\r[");
            for (int i = 0; i < BAR_WIDTH; ++i)
            {
                if (i < pos)
                    printf("=");
                else if (i == pos)
                    printf(">");
                else
                    printf(" ");
            }
            printf("] %.1f%%", progress * 100);
            fflush(stdout);
        }
    }
    printf("\n"); // Переход на новую строку после завершения

    fclose(file);
}

int mock_sd_init()
{
    FILE *file = fopen(pathToSD, "w+b");
    if (file == NULL)
    {
        puts("Isn't open file!\n");
        return -1;
    }

    return 0;
}