
#pragma once 

#include <stdint.h>

typedef struct {
    uint16_t year;
    uint16_t month;
    uint16_t day;
} DateType;


typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} TimeType;


int get_cur_time_and_date(DateType *date, TimeType *time);