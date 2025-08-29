#pragma once

#include <stddef.h>
#include <stdint.h>




// Тип функции для выделения памяти
typedef void* (*fat32_malloc_t)(size_t size);

// Тип функции для освобождения памяти
typedef int (*fat32_free_t)(void *ptr, size_t size);

// Тип инициализации аллокатора памяти
typedef void (*fat32_alloc_init_t)(void);



typedef struct {
    fat32_malloc_t alloc;
    fat32_free_t free;
    fat32_alloc_init_t allocator_init;
} Fat32Allocator;

// Инициализация аллокатора
void fat32_allocator_init(Fat32Allocator *custom_allocator);

// Функции-обертки
void* fat32_alloc(size_t size);
int   fat32_free(void *ptr, size_t size);

