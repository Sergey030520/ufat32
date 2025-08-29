#include "fat32/pool_memory.h"
#include <string.h>


// 5440
#define POOL_SIZE 4096
#define BLOCK_SIZE 1
#define BLOCKS_COUNT (POOL_SIZE / BLOCK_SIZE)

static uint8_t memory_pool[POOL_SIZE];
static uint8_t bitmap[(BLOCKS_COUNT + 7) / 8];
static size_t last_free_block = 0;
static uint8_t initialized = 0;



void *pool_alloc(uint32_t size)
{
    const uint16_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (blocks_needed > BLOCKS_COUNT)
    {
        return NULL;
    }

    uint16_t consecutive_free = 0;
    uint16_t first_free_block = 0;
    uint16_t search_start = last_free_block;

    for (int pass = 0; pass < 2; pass++)
    {
        for (uint16_t idxBlock = search_start;
             idxBlock < (pass ? last_free_block : BLOCKS_COUNT);
             idxBlock++)
        {

            if ((bitmap[idxBlock / 8] & (1 << (idxBlock % 8))) == 0)
            {
                if (consecutive_free == 0)
                {
                    first_free_block = idxBlock;
                }

                if (++consecutive_free >= blocks_needed)
                {
                    for (uint16_t i = first_free_block; i < first_free_block + blocks_needed; i++)
                    {
                        bitmap[i / 8] |= (1 << (i % 8));
                    }

                    last_free_block = first_free_block + blocks_needed;
                    while (last_free_block < BLOCKS_COUNT &&
                           (bitmap[last_free_block / 8] & (1 << (last_free_block % 8))))
                    {
                        last_free_block++;
                    }

                    if (last_free_block >= BLOCKS_COUNT)
                    {
                        last_free_block = 0;
                    }

                    return &memory_pool[first_free_block * BLOCK_SIZE];
                }
            }
            else
            {
                consecutive_free = 0;
            }
        }

        search_start = 0;
        consecutive_free = 0;
    }

    return NULL;
}

void pool_init()
{
    if (initialized)
        return;
    last_free_block = 0;
    initialized = 0x1;
    memset(bitmap, 0, sizeof(bitmap));
}

void pool_free()
{
    if (initialized == 0x0)
        return;
    memset(bitmap, 0x0, sizeof(bitmap));
    memset(memory_pool, 0x0, sizeof(memory_pool));
    last_free_block = 0;
    initialized = 0;
}
int pool_free_region(void *ptr, uint32_t size)
{
    if (ptr == NULL || size == 0)
    {
        return POOL_ERR_INVALID_ARGUMENT;
    }

    uint8_t *start = (uint8_t *)ptr;
    if (start < memory_pool || start >= memory_pool + POOL_SIZE)
    {
        return POOL_ERR_POINTER_OUT_OF_RANGE;
    }

    uint16_t first_block = (start - memory_pool) / BLOCK_SIZE;
    uint16_t blocks_to_free = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    uint16_t last_block = first_block + blocks_to_free - 1;

    if (last_block >= BLOCKS_COUNT)
    {
        return POOL_ERR_OUT_OF_MEMORY;
    }

    for (uint16_t i = first_block; i <= last_block; i++)
    {
        if ((bitmap[i / 8] & (1 << (i % 8))))
        {
            bitmap[i / 8] &= ~(0x1 << (i % 8));
        }
        if (i < last_free_block)
        {
            last_free_block = first_block;
        }
    }

    return 0;
}