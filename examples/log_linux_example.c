#include "log_linux_example.h"


void linux_log(Fat32LogLevel level, const char *file, int line, const char *format, va_list args)
{
    const char *level_str = "";
    switch(level) {
        case FAT32_LOG_INFO:  level_str = "INFO"; break;
        case FAT32_LOG_WARN:  level_str = "WARN"; break;
        case FAT32_LOG_ERROR: level_str = "ERROR"; break;
    }

    printf("[%s] %s:%d: ", level_str, file, line);
    vprintf(format, args); 
    printf("\n");
}


