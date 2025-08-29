#include "fat32/fat32_time.h"


void fat32_date_from_fat(uint16_t fat_date, FAT32_Date_Type *date)
{
    if (!date) return;
    date->year  = 1980 + FAT32_GET_YEAR(fat_date);
    date->month = FAT32_GET_MONTH(fat_date);
    date->day   = FAT32_GET_DAY(fat_date);
}

void fat32_date_to_fat(const FAT32_Date_Type *date, uint16_t *fat_date)
{
    if (!date || !fat_date) return;
    *fat_date = FAT32_SET_DATE(date->day, date->month, date->year - 1980);
}


void fat32_time_from_fat(uint16_t fat_time, FAT32_Time_Type *time)
{
    if (!time) return;
    time->hour   = FAT32_GET_HOUR(fat_time);
    time->minute = FAT32_GET_MINUTE(fat_time);
    time->second = FAT32_GET_SEC(fat_time) * 2; // шаг 2 секунды
}

void fat32_time_to_fat(const FAT32_Time_Type *time, uint16_t *fat_time)
{
    if (!time || !fat_time) return;
    *fat_time = FAT32_SET_TIME(time->hour, time->minute, time->second / 2);
}