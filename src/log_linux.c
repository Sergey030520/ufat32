#include "fat32/log_fat32.h"
#include <stdio.h>
#include <stdarg.h>


void fat32_log(Fat32LogLevel level, const char *file, int line, const char *format, ...)
{
    const char *level_str = "";
    switch(level) {
        case FAT32_LOG_INFO:  level_str = "INFO"; break;
        case FAT32_LOG_WARN:  level_str = "WARN"; break;
        case FAT32_LOG_ERROR: level_str = "ERROR"; break;
    }

    va_list args;
    va_start(args, format);
    printf("[%s] %s:%d: ", level_str, file, line);
    vprintf(format, args);
    printf("\n");
    va_end(args);
}
