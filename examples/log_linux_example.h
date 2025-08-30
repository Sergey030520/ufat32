#include "fat32/log_fat32.h"
#include <stdio.h>
#include <stdarg.h>



void linux_log(Fat32LogLevel level, const char *file, int line, const char *format, va_list args);