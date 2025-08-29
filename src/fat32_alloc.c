#include "fat32/fat32_alloc.h"
#include <stdlib.h>
#include <string.h>


static Fat32Allocator fat32_allocator = {0};



// Для стандартных malloc и free
static void *default_malloc(size_t size)
{
    return malloc(size);
}
static int default_free(void *ptr, size_t size)
{
    (void)size;
    if(ptr != NULL) free(ptr);
    return 0;
}

void fat32_allocator_init(Fat32Allocator *custom_allocator)
{
    if (custom_allocator && custom_allocator->alloc && custom_allocator->free)
    {
        fat32_allocator = *custom_allocator;
        if (fat32_allocator.allocator_init)
            fat32_allocator.allocator_init();
    }
    else
    {
        fat32_allocator.alloc = default_malloc;
        fat32_allocator.free = default_free;
    }
}

void* fat32_alloc(size_t size)
{
    if (!fat32_allocator.alloc)
        return NULL;
    void *ptr = fat32_allocator.alloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

int fat32_free(void *ptr, size_t size)
{
    if (!fat32_allocator.free)
        return -1;
    return fat32_allocator.free(ptr, size);
}

