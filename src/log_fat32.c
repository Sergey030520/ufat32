#include "fat32/log_fat32.h"
#include <stdio.h>


static Fat32LogCallback current_logger = NULL;

void fat32_set_logger(Fat32LogCallback logger) {
    current_logger = logger;
}

void fat32_log(Fat32LogLevel level, const char *file, int line, const char *format, ...) {
    if (!current_logger) return;

    va_list args;
    va_start(args, format);
    current_logger(level, file, line, format, args);
    va_end(args);
}
