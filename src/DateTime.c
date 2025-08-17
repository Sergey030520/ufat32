
#include "DateTime.h"

#include <time.h>

int get_cur_time_and_date(DateType *date_val, TimeType *time_val)
{
    if(date_val == NULL || time_val == NULL) return -1;
    time_t raw_time;
    struct  tm *time_info;
    
    time(&raw_time);

    time_info = localtime(&raw_time);

    date_val->year = time_info->tm_year;
    date_val->month = time_info->tm_mon;
    date_val->day = time_info->tm_mday;
    
    time_val->hour = time_info->tm_hour; 
    time_val->minute = time_info->tm_min; 
    time_val->second = time_info->tm_sec; 

    return 0;
}