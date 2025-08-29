#pragma once

#include "fat32_types.h"
#include "block_device.h"



/**
 * debug_print_mbr
 * ----------------
 * Считывает MBR с устройства и выводит его содержимое для отладки.
 *
 * @param dev - указатель на устройство хранения (BlockDevice)
 *
 * @return 0 при успехе, -1 при ошибке
 */
int debug_print_mbr(BlockDevice *dev);



/**
 * debug_print_fsinfo
 * ------------------
 * Считывает сектор FSInfo с устройства и выводит его содержимое для отладки.
 *
 * @param dev           Устройство хранения (BlockDevice)
 * @param fsinfo_sector Номер сектора FSInfo (обычно берётся из BPB_FSInfo)
 * @param bytes_per_sec Размер сектора в байтах (обычно 512)
 *
 * @return 0 при успехе, -1 при ошибке
 */
int debug_print_fsinfo(BlockDevice *dev, uint32_t fsinfo_sector, uint16_t bytes_per_sec);



/**
 * Выводит содержимое директории в указанном секторе
 * @param dev          - устройство хранения
 * @param sector       - номер сектора
 * @param bytes_per_sec - размер сектора в байтах (из BPB)
 * @return 0 при успехе, -1 при ошибке
 */
int debug_dump_dir_sector(BlockDevice *dev, uint32_t sector, uint16_t bytes_per_sec);

/**
 * debug_print_fat_sector
 * ----------------------
 * Выводит содержимое одного сектора таблицы FAT32 в удобочитаемом виде.
 * Используется для отладки работы с FAT32.
 *
 * @param dev           Указатель на устройство хранения (BlockDevice).
 * @param sector        Номер сектора FAT, который нужно прочитать.
 * @param bytes_per_sec Размер одного сектора в байтах (обычно берётся из BPB).
 *
 * @return 0 при успешном чтении и выводе сектора,
 *        -1 если произошла ошибка (устройство не инициализировано или не удалось прочитать сектор).
 *
 * Примечания:
 * - Функция выводит каждый 32-битный элемент FAT в формате 0xXXXXXXXX.
 * - Рекомендуется вызывать только после монтирования файловой системы,
 *   когда известен размер сектора (bytes_per_sec).
 */
int debug_print_fat_sector(BlockDevice *dev, uint32_t sector, uint16_t bytes_per_sec);


/**
 * Выводит короткую запись каталога (SFN) для отладки
 */
void debug_print_sfn_entry(const FatDir_Type *file);

/**
 * Выводит длинную запись каталога (LFN) для отладки
 */
void debug_print_lfn_entry(const LDIR_Type *entry);