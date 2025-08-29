#include <stdio.h>
#include <stdlib.h>
#include "fat32/FAT32.h"
#include "mock_sd.h"
#include <memory.h>
#include "fat32/log_fat32.h"
#include "fat32/fat32_alloc.h"


Fat32Allocator allocator = {0};


void show_file_info(FAT32_File *file)
{
    if (file == NULL)
        return;
    printf("cluster: %d, cluster idx: %d, sector_idx: %d, offset: %d\n", file->position.cluster_number,
           file->position.cluster_idx, file->position.sector_idx, file->position.byte_offset);
}

#define DEFAULT_TIMEOUT 1000

int clear_sd(uint32_t sector_num, uint32_t sector_count, uint32_t sector_size);
int read_sd(uint8_t *buffer, uint32_t sector_count, uint32_t start_sector, uint32_t sector_size);
int write_sd(const uint8_t *data, uint32_t sector_count, uint32_t start_sector, uint32_t sector_size);
int init_fat32(BlockDevice *device);


int main()
{
    int status = 0;
    

    BlockDevice device = {
        .read = read_sd,
        .write = write_sd,
        .clear = clear_sd,
        .block_size=512};
    // clear_sd(0, (uint32_t)(SIZE_8GB/512));
    status = init_fat32(&device);
    if(status != 0){
        FAT32_LOG_INFO("error init fat32: %d\n", status);
        return 0;
    }
    // // // uint32_t cluster;
    // // // clear_fat32();
    // // // uint16_t entryNumb = (26 + 13 - 1) / 13;
    // // // printf("%d\n", entryNumb);
     status = mkdir_fat32("/MYDIR");
    if (status == 0)
    {
        printf("folder create!\n");
    }
    else
    {
        printf("error: %d\n", status);
    }
    status = mkdir_fat32("/MYDIR/TEST1/test1");
    if (status == 0)
    {
        printf("folder create!\n");
    }
    else
    {
        printf("error: %d\n", status);
    }
    FAT32_File *file = NULL;
    status = open_file_fat32("/MYDIR/TEST1/test1/1.txt", &file, F_WRITE);
    if(status != 0 || file == NULL){
        printf("file is not open: %d\n", status);
    }
    status = path_exists_fat32("/MYDIR/TEST1/test1/1.txt");
    if(status != 0){
        printf("file is not exists!\n");
    }

    char buffer[] = "How do you do?";
    status = write_file_fat32(file, buffer, strlen(buffer));
    if(status < 0){
        printf("write error: %d\n", status);
    }else{
        printf("write: %d\n", status);
        status = flush_fat32(file);
        if(status != 0){
            printf("error flush: %d\n", status);
        }
        status = close_file_fat32(&file);
        if(status < 0){
            printf("error close file: %d\n", status);
        }
    }

     status = open_file_fat32("/MYDIR/TEST1/test1/1.txt", &file, F_READ);
    if(status != 0 || file == NULL){
        printf("file is not open: %d\n", status);
    }
    char buffer_rx[256];
    status = read_file_fat32(file, buffer_rx, 256);
    if(status < 0){
         printf("error read: %d\n", status);
    }else{
        buffer_rx[status+1] = '\0';
        printf("read_text: %s\n", buffer_rx);
    }
    return 0;
}

#include "fat32/pool_memory.h"

int init_fat32(BlockDevice *device)
{
    if (device == NULL)
    {
        return -1;
    }
    
    allocator.alloc = pool_alloc;
    allocator.free = pool_free_region;
    allocator.allocator_init = pool_init;
    fat32_allocator_init(NULL);


    int status = mount_fat32(device);
    if (status != 0)
    {
        FAT32_LOG_INFO("SD card is not formatted\r\n");
        FAT32_LOG_INFO("Formatting SD card\r\n");
        status = formatted_fat32(device, SIZE_8GB);
        if (status != 0)
        {
            return -2;
        }
        FAT32_LOG_INFO("Retrying SD card mount\r\n");

        status = mount_fat32(device);
        if (status != 0)
        {
            return -3;
        }
    }


    return 0;
}

int read_safe_sd(uint8_t *buffer, uint32_t count_blocks, uint32_t block_start, uint32_t timeout)
{
    int attempts = 5;
    int status;
    while (attempts--)
    {
        status = mock_sd_read(buffer, count_blocks, block_start, timeout);
        if (status == 0)
            return 0; 
    }
    return status;
}
int read_sd(uint8_t *buffer, uint32_t sector_count, uint32_t start_sector, uint32_t sector_size)
{
    uint32_t block_size = 512;
    uint32_t block_start = (start_sector * sector_size) / block_size;
    uint32_t total_bytes = sector_count * sector_size;

    uint32_t full_blocks = total_bytes / block_size;
    uint32_t remaining_bytes = total_bytes % block_size;

    if (full_blocks > 0)
    {
        int status = read_safe_sd(buffer, full_blocks, block_start, DEFAULT_TIMEOUT);
        if (status != 0) return status;
    }

    if (remaining_bytes > 0)
    {
        uint8_t *tmp_block = fat32_alloc(block_size);
        if (tmp_block == NULL) return POOL_ERR_ALLOCATION_FAILED;

        int status = read_safe_sd(tmp_block, 1, block_start + full_blocks, DEFAULT_TIMEOUT);
        if (status != 0) 
        {
            fat32_free(tmp_block, block_size);
            return status;
        }

        memcpy(buffer + full_blocks * block_size, tmp_block, remaining_bytes);
        fat32_free(tmp_block, block_size);
    }

    return 0;
}

int write_safe_sd(const uint8_t *data, uint32_t count_blocks, uint32_t block_start, uint32_t timeout)
{
    int attempts = 3;
    int status;
    while (attempts--)
    {
        status = mock_sd_write(data, count_blocks, block_start, timeout);
        if (status == 0)
            return 0;
        
        else{ FAT32_LOG_INFO("err_status:%d\r\n", status);}
        // delay_timer(100);
    }
    return status;
}

int write_sd(const uint8_t *data, uint32_t sector_count, uint32_t start_sector, uint32_t sector_size)
{
    uint32_t block_size = 512;
    uint32_t block_start = (start_sector * sector_size) / block_size;
    uint32_t total_bytes = sector_count * sector_size;

    uint32_t full_blocks = total_bytes / block_size;
    uint32_t remaining_bytes = total_bytes % block_size;

    int status = 0;

    if (full_blocks > 0)
    {
        status = write_safe_sd(data, full_blocks, block_start, DEFAULT_TIMEOUT);
        if (status != 0) return status;
    }

    if (remaining_bytes > 0)
    {
        uint8_t *tmp_block = fat32_alloc(block_size);
        if (!tmp_block) return -1;

        status = mock_sd_read(tmp_block, 1, block_start + full_blocks, DEFAULT_TIMEOUT);
        if (status != 0)
        {
            fat32_free(tmp_block, block_size);
            return status;
        }

        memcpy(tmp_block, data + full_blocks * block_size, remaining_bytes);

        status = mock_sd_write(tmp_block, 1, block_start + full_blocks, DEFAULT_TIMEOUT);
        fat32_free(tmp_block, block_size);

        if (status != 0) return status;
    }

    return 0;
}


int clear_sd(uint32_t sector_num, uint32_t sector_count, uint32_t sector_size)
{
    uint32_t block_start = (sector_num * sector_size) / 512;
    uint32_t offset = (sector_count * sector_size + 512 - 1) / 512;
    return mock_sd_erase(block_start, offset);
}

