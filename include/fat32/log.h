#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
    FAT32_LOG_INFO,
    FAT32_LOG_WARN,
    FAT32_LOG_ERROR
} Fat32LogLevel;


void fat32_log(Fat32LogLevel level, const char *file, int line, const char *format, ...);


#define FAT32_LOG_INFO(format, ...)  fat32_log(FAT32_LOG_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define FAT32_LOG_WARN(format, ...)  fat32_log(FAT32_LOG_WARN, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define FAT32_LOG_ERROR(format, ...) fat32_log(FAT32_LOG_ERROR, __FILE__, __LINE__, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif
