#include "log.h"
#include "stm_log.h"
#include <stdarg.h>

void fat32_log(Fat32LogLevel level, const char *file, int line, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    switch(level) {
        case FAT32_LOG_INFO:  stm_log(LEVEL_INFO, 0, file, line, format, args); break;
        case FAT32_LOG_WARN:  stm_log(LEVEL_WARN, 0, file, line, format, args); break;
        case FAT32_LOG_ERROR: stm_log(LEVEL_ERROR, 0, file, line, format, args); break;
    }

    va_end(args);
}
