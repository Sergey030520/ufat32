#pragma once


#include <stdint.h>
#include "fat32_types.h"


/**
 * Преобразует 16-битное поле FAT32 даты в структуру FAT32_Date_Type
 * @param fat_date 16-битное поле даты FAT32 (например, entry.DIR_WrtDate)
 * @param date     указатель на структуру FAT32_Date_Type для записи результата
 */
void fat32_date_from_fat(uint16_t fat_date, FAT32_Date_Type *date);

/**
 * Преобразует структуру FAT32_Date_Type в 16-битное поле FAT32 даты
 * @param date     указатель на структуру FAT32_Date_Type
 * @param fat_date указатель на 16-битное поле FAT32 для записи результата
 */
void fat32_date_to_fat(const FAT32_Date_Type *date, uint16_t *fat_date);

/**
 * Преобразует 16-битное поле FAT32 времени в структуру FAT32_Time_Type
 * @param fat_time 16-битное поле времени FAT32 (например, entry.DIR_WrtTime)
 * @param time     указатель на структуру FAT32_Time_Type для записи результата
 */
void fat32_time_from_fat(uint16_t fat_time, FAT32_Time_Type *time);

/**
 * Преобразует структуру FAT32_Time_Type в 16-битное поле FAT32 времени
 * @param time     указатель на структуру FAT32_Time_Type
 * @param fat_time указатель на 16-битное поле FAT32 для записи результата
 */
void fat32_time_to_fat(const FAT32_Time_Type *time, uint16_t *fat_time);

