#include "fat32/FAT32.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "fat32/fat32_time.h"
#include "fat32/fat32_alloc.h"
#include "fat32/file_utils.h"
#include "fat32/log_fat32.h"

FatLayoutInfo *fat_info = NULL;

void *stm_memcpy(void *dest, const void *src, uint32_t size);

// ===============================
// Directory entry handling
// ===============================
int find_entry_cluster_fat32(char *name, uint32_t length, uint32_t parent_cluster, uint32_t *out_cluster);
int get_attr_entry_fat32(uint32_t cluster_parent, uint32_t child_cluster, uint8_t *attr);
int find_free_dir_entries(uint32_t parent_cluster, const uint16_t entry_count, DirEntryPosition *position);
int make_lfn_entries(const char *name, uint32_t length, uint8_t chkSum, LDIR_Type *entries, uint16_t numb_entries);
FAT32_File *init_file_handle(const char *file_name, uint32_t parent_cluster, uint8_t mode);
int read_directory_entry_fat32(DirEntryPosition *position, FatDir_Type *entry);
int write_dir_entries_at(const DirEntryPosition *position, const void *entries, uint16_t entry_count);
int is_free_entry_fat32(FatDir_Type *entry);
int create_dir_fat32(char *name, uint32_t name_length, uint32_t parent_cluster); // create a new directory

// ===============================
// Cluster management
// ===============================
int extend_cluster_chain_if_needed(uint32_t *last_cluster);
int allocate_cluster_fat32(uint32_t *new_cluster);
int get_next_cluster_fat32(uint32_t *prev_cluster);
int is_dir_empty_fat32(uint32_t cluster);
int update_fat32(uint32_t cluster, uint32_t value);
int find_free_cluster(uint32_t *free_cluster);
void join_cluster_number(uint32_t *cluster, uint16_t high, uint16_t low);
void split_cluster_number(uint32_t cluster, uint16_t *high, uint16_t *low);

// ===============================
// File name handling
// ===============================
int fat32_compare_sfn(char *name, uint32_t length, const FatDir_Type *entry);

/**
 * Извлекает путь к директории из полного пути к файлу/каталогу.
 *
 * Например, из "/folder1/folder2/file.txt" вернёт "/folder1/folder2/".
 *
 * @param path      Полный путь к файлу.
 * @param dir_path  Буфер, куда будет записан путь к директории.
 * @param size      Размер буфера dir_path.
 * @return 0 при успехе,
 *          FAT32_ERR_INVALID_ARGUMENT если аргументы некорректны,
 *         FAT32_ERR_INVALID_PATH если не найден символ '/',
 *         FAT32_ERR_ALLOC_FAILED при ошибке выделения памяти.
 */
int get_dir_path(char *path, char *dir_path, int size)
{
    if (path == NULL || dir_path == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    int length = strlen(path);
    int status = 0;

    char *pathToFile = fat32_alloc(length);
    if (pathToFile == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    // Если в конце пути указан / заменяем на конец строки
    strcpy(pathToFile, path);
    if (length > 0 && pathToFile[length - 1] == '/')
        *(pathToFile + (length - 1)) = '\0';

    // Ищем последний символ '/'
    char *last_slash = fat32_find_last_char(pathToFile, '/');
    if (last_slash == NULL)
    {
        status = fat32_free(pathToFile, length);
        if (status != 0)
        {
            // вывести в лог
        };
        return FAT32_ERR_INVALID_PATH;
    }

    // Извлекаем путь до директории
    size_t path_len = last_slash - pathToFile + 1;
    strncpy(dir_path, pathToFile, path_len);
    dir_path[path_len] = '\0';
    status = fat32_free(pathToFile, length);
    if (status != 0)
    {
        // вывести в лог
    };
    return 0;
}

/**
 * Извлекает последнюю компоненту (имя файла или папки) из указанного пути.
 *
 * Например, из "/folder1/folder2/file.txt" вернёт "file.txt".
 *
 * @param path           Полный путь.
 * @param name_component Буфер для записи последней компоненты пути.
 * @return 0 при успехе,
 *         FAT32_ERR_INVALID_ARGUMENT если path или name_component равны NULL,
 *         FAT32_ERR_INVALID_PATH если в пути не найдено ни одного '/',
 *         FAT32_ERR_ALLOC_FAILED при ошибке выделения памяти.
 */
int get_last_path_component(char *path, char *name_component)
{
    if (path == NULL || name_component == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    int length = strlen(path);
    int status = 0;
    char *pathToFile = fat32_alloc(length);
    if (pathToFile == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    strcpy(pathToFile, path);
    if (length > 0 && pathToFile[length - 1] == '/')
    {
        pathToFile[length - 1] = '\0';
    }

    // Ищем последнее вхождение символа '/'
    char *last_slash = fat32_find_last_char(pathToFile, '/');
    if (last_slash == NULL)
    {
        status = fat32_free(pathToFile, length);
        if (status != 0)
        {
            // вывести в лог
        }
        return FAT32_ERR_INVALID_PATH;
    }
    // Копируем имя файла/директории в name_component
    strcpy(name_component, last_slash + 1);

    status = fat32_free(pathToFile, length);
    if (status != 0)
    {
        // вывести в лог
    }

    return 0;
}

/**
 * Копирует заданное количество байт из источника в назначение.
 *
 * Не зависит от стандартной библиотеки C.
 *
 * @param dest  Указатель на буфер назначения.
 * @param src   Указатель на источник данных.
 * @param size  Количество байт для копирования.
 * @return Указатель на буфер назначения, или NULL, если dest и src равны NULL.
 */
void *stm_memcpy(void *dest, const void *src, uint32_t size)
{
    if ((dest == 0) && (src == 0))
    {
        return 0;
    }
    uint8_t *dest_local = dest;
    const uint8_t *src_local = src;
    while (size-- > 0)
    {
        *dest_local = *src_local;
        ++dest_local;
        ++src_local;
    }
    return dest;
}

/**
 * Вычисляет размер таблицы FAT32 в секторах.
 *
 * Размер рассчитывается на основе общего объёма и структуры MBR (Main Boot Record),
 * включая количество кластеров и размер одного кластера.
 *
 * @param capacity   Общий объём накопителя в байтах.
 * @param mbr_data   Указатель на структуру MBR_Type с параметрами разметки.
 *
 * @return Размер FAT-таблицы в секторах.
 *         0 — в случае некорректных входных данных (например, ёмкость меньше зарезервированных секторов).
 */
uint32_t calculate_size_table_fat32(uint64_t capacity, MBR_Type *mbr_data)
{
    if (!mbr_data)
        return 0;

    uint32_t bytes_per_sector = mbr_data->BPB_BytsPerSec;
    uint32_t sec_per_clus = mbr_data->BPB_SecPerClus;
    uint32_t reserved_sectors = mbr_data->BPB_RsvdSecCnt;
    uint32_t num_fats = mbr_data->BPB_NumFATs;

    FAT32_LOG_INFO("bytes_per_sector: %u, sec_per_clus: %u, reserved_sectors: %u, num_fats: %u\r\n",
                   bytes_per_sector, sec_per_clus, reserved_sectors, num_fats);

    uint64_t total_sectors = capacity / bytes_per_sector;
    if (total_sectors <= reserved_sectors)
        return 0;

    uint64_t data_sectors = total_sectors - reserved_sectors;
    uint64_t data_clusters = data_sectors / sec_per_clus;

    uint64_t fat_sectors = (data_clusters * 4 + bytes_per_sector - 1) / bytes_per_sector;

    if (fat_sectors > 0xFFFFFFFF)
        return 0; // защита от переполнения

    return (uint32_t)fat_sectors;
}

int seek_file_fat32(FAT32_File *file, int32_t offset, SEEK_Mode mode)
{
    if (fat_info == NULL || file == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    int32_t position = 0;
    uint32_t bytes_per_cluster = fat_info->bytesPerSec * fat_info->secPerClus;
    uint32_t count_cluster = offset / bytes_per_cluster;
    int status = 0;

    if (mode == F_SEEK_SET)
    {
        position = offset;
    }
    else if (mode == F_SEEK_CUR)
    {
        position = file->position.cluster_idx * fat_info->bytesPerSec * fat_info->secPerClus + file->position.sector_idx * fat_info->bytesPerSec + file->position.byte_offset + offset;
    }
    else if (mode == F_SEEK_END)
    {
        position = file->size_bytes + offset;
    }
    else
    {
        return FAT32_ERR_INVALID_SEEK_MODE;
    }

    if (position > file->size_bytes || position < 0)
    {
        return FAT32_ERR_INVALID_POSITION;
    }

    if (mode == F_SEEK_SET || mode == F_SEEK_END)
    {
        file->position.cluster_number = file->first_cluster;
        file->position.cluster_idx = 0;
        file->position.sector_idx = 0;
        file->position.byte_offset = 0;
    }

    uint32_t next_cluster = file->position.cluster_number;
    uint32_t offset_in_cluster = position % bytes_per_cluster;

    while (file->position.cluster_idx != count_cluster)
    {
        status = get_next_cluster_fat32(&next_cluster);
        if (status == -1 || next_cluster == FILE_END_TABLE_FAT32)
        {
            return FAT32_ERR_CLUSTER_CHAIN_BROKEN;
        }
        file->position.cluster_idx++;
    }

    file->position.cluster_number = next_cluster;
    file->position.sector_idx = offset_in_cluster / fat_info->bytesPerSec;
    file->position.byte_offset = offset_in_cluster % fat_info->bytesPerSec;
    return 0;
}

/**
 * Освобождает цепочку кластеров в FAT32, начиная с указанного кластера.
 *
 * Функция проходит по цепочке кластеров, начиная с `cluster`, и помечает каждый кластер как свободный.
 *
 * @param cluster Начальный кластер для освобождения.
 * @return 0 при успешном освобождении,
 *         FAT32_ERR_FS_NOT_LOADED если файловая система не инициализирована,
 *         код ошибки из get_next_cluster_fat32 при ошибке чтения,
 *         FAT32_ERR_UPDATE_FAILED при ошибке обновления таблицы FAT.
 */
int free_cluster_fat32(uint32_t cluster)
{
    if (fat_info == NULL)
        return FAT32_ERR_FS_NOT_LOADED;

    uint32_t next_cluster = 0;
    int status = 0;
    while (1)
    {
        next_cluster = cluster;
        status = get_next_cluster_fat32(&next_cluster);
        if (status != 0)
        {
            return status;
        }
        if (next_cluster == FILE_END_TABLE_FAT32)
        {
            break;
        }
        status = update_fat32(cluster, NOT_USED_CLUSTER_FAT32);
        if (status != 0)
        {
            return FAT32_ERR_UPDATE_FAILED;
        }
        cluster = next_cluster;
    }

    // Освободить последний кластер
    status = update_fat32(cluster, NOT_USED_CLUSTER_FAT32);
    if (status != 0)
    {
        return FAT32_ERR_UPDATE_FAILED;
    }
    return 0;
}

/**
 * Ищет запись файла или папки по имени в указанном родительском кластере.
 *
 * Функция перебирает все сектора в кластере и его цепочке, обрабатывая как обычные имена (8.3),
 * так и длинные имена (LFN). Если запись найдена, позиция сохраняется в структуре entry_pos.
 *
 * @param name           Имя файла или папки, которое необходимо найти (длинное имя поддерживается).
 * @param parent_cluster Кластер каталога, в котором производится поиск.
 * @param entry_pos      Указатель на структуру DirEntryPosition, в которую будет записано положение найденной записи.
 *
 * @return 0 — если запись найдена,
 *         FAT32_ERR_INVALID_ARGUMENT — если указаны некорректные параметры (например, name == NULL),
 *         FAT32_ERR_ENTRY_NOT_FOUND — если запись с заданным именем не найдена,
 *         FAT32_ERR_ENTRY_CORRUPTED — если структура LFN повреждена или нарушена,
 *         FAT32_ERR_READ_FAIL — если произошла ошибка чтения с SD-карты,
 *         FAT32_ERR_ALLOC_FAILED — если не удалось выделить буфер из пула памяти.
 */
int find_entry_by_name(const char *name, uint32_t parent_cluster, DirEntryPosition *entry_pos)
{
    if (name == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    uint32_t current_cluster = parent_cluster;
    uint32_t address = (current_cluster - fat_info->root_cluster) * fat_info->secPerClus + fat_info->address_region;
    uint32_t sector = 0, idxEntry = 0;
    int status = 0;

    FatDir_Type *entry;
    uint8_t lfn_active = 0;
    uint8_t check_sum = 0;

    uint8_t idx = 0;
    uint8_t order = 0;
    uint32_t length = strlen(name);
    uint16_t buffer_name[MAX_NAME_SIZE] = {0};

    while (current_cluster != FILE_END_TABLE_FAT32)
    {
        address = (current_cluster - fat_info->root_cluster) * fat_info->secPerClus + fat_info->address_region;

        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            status = fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (idxEntry = 0; idxEntry < fat_info->bytesPerSec; idxEntry += sizeof(FatDir_Type))
            {
                entry = (FatDir_Type *)&buffer[idxEntry];
                if (entry->DIR_Name[0] == ENTRY_FREE_FULL_FAT32)
                {
                    status = FAT32_ERR_ENTRY_NOT_FOUND;
                    goto cleanup;
                }
                else if (entry->DIR_Name[0] == ENTRY_FREE_FAT32)
                {
                    if (lfn_active)
                    {
                        status = FAT32_ERR_ENTRY_CORRUPTED;
                        goto cleanup;
                    }
                    continue;
                }
                else if ((entry->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
                {
                    LDIR_Type *lfn_entry = (LDIR_Type *)entry;
                    if (lfn_entry->LDIR_Ord & LFN_ENTRY_LAST)
                        check_sum = lfn_entry->LDIR_Chksum;
                    if (lfn_entry->LDIR_Type != 0)
                        continue;
                    order = lfn_entry->LDIR_Ord & ~LFN_ENTRY_LAST;

                    idx = (order - 1) * MAX_SYMBOLS_ENTRY;
                    stm_memcpy((uint8_t *)(&buffer_name[idx]), lfn_entry->LDIR_Name1, sizeof(lfn_entry->LDIR_Name1));
                    idx += sizeof(lfn_entry->LDIR_Name1) / 2;
                    stm_memcpy((uint8_t *)&buffer_name[idx], lfn_entry->LDIR_Name2, sizeof(lfn_entry->LDIR_Name2));
                    idx += sizeof(lfn_entry->LDIR_Name2) / 2;
                    stm_memcpy((uint8_t *)&buffer_name[idx], lfn_entry->LDIR_Name3, sizeof(lfn_entry->LDIR_Name3));
                    lfn_active = 1;
                }
                else
                {
                    if (lfn_active)
                    {
                        if (fat32_sfn_checksum((uint8_t *)entry->DIR_Name) == check_sum)
                        {
                            if (fat32_compare_lfn(name, buffer_name) == 0)
                            {
                                entry_pos->cluster = parent_cluster;
                                entry_pos->sector = sector;
                                entry_pos->offset = (idxEntry / sizeof(LDIR_Type));
                                status = 0;
                                goto cleanup;
                            }
                        }
                        lfn_active = 0;
                        check_sum = 0;
                        memset(buffer_name, 0x00, sizeof(buffer_name));
                    }
                    else if (fat32_compare_sfn(name, length, entry) == 0)
                    {
                        entry_pos->cluster = parent_cluster;
                        entry_pos->sector = sector;
                        entry_pos->offset = idxEntry / sizeof(FatDir_Type);
                        status = 0;
                        goto cleanup;
                    }
                }
            }
            status = get_next_cluster_fat32(&current_cluster);
            if (status != 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
        }
    }
    status = FAT32_ERR_ENTRY_NOT_FOUND;

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * Инициализирует дескриптор файла FAT32 для работы с файлом.
 *
 * @param file_name       Имя файла для открытия.
 * @param parent_cluster  Кластер родительской директории.
 * @param mode            Режим доступа: F_READ, F_WRITE или F_APPEND.
 *
 * @return Указатель на структуру FAT32_File или NULL при ошибке.
 */
FAT32_File *init_file_handle(const char *file_name, uint32_t parent_cluster, uint8_t mode)
{
    if (file_name == NULL)
    {
        return NULL;
    }

    // Поиск записи директории по имени файла
    DirEntryPosition position;
    int status = find_entry_by_name(file_name, parent_cluster, &position);
    if (status != 0)
    {
        return NULL;
    }

    // Чтение записи директории по найденной позиции
    FatDir_Type entry = {0};
    status = read_directory_entry_fat32(&position, &entry);
    if (status != 0)
    {
        return NULL;
    }

    // Выделение памяти из пула и инициализация дескриптора
    FAT32_File *desc = fat32_alloc(sizeof(FAT32_File));
    if (desc == NULL)
    {
        return NULL;
    }

    memset((uint8_t *)desc, 0, sizeof(FAT32_File));
    join_cluster_number(&desc->first_cluster, entry.DIR_FstClusHI, entry.DIR_FstClusLO);
    stm_memcpy((uint8_t *)&desc->entry_pos, (uint8_t *)&position, sizeof(DirEntryPosition));

    if (mode == F_READ)
    {
        desc->size_bytes = entry.DIR_FileSize;
        desc->flags = F_READ;
        status = seek_file_fat32(desc, 0, F_SEEK_SET);
    }
    else if (mode == F_WRITE)
    {
        // Очистка последующих кластеров, если они есть
        uint32_t next_cluster = desc->first_cluster;
        while (1)
        {
            status = get_next_cluster_fat32(&next_cluster);
            if (status != 0)
            {
                goto cleanup;
            }

            if (next_cluster == FILE_END_TABLE_FAT32)
            {
                break;
            }

            status = free_cluster_fat32(next_cluster);
            if (status != 0)
            {
                goto cleanup;
            }
        }

        desc->flags = F_WRITE;
        desc->size_bytes = 0;
        status = seek_file_fat32(desc, 0, F_SEEK_SET);
    }
    else if (mode == F_APPEND)
    {
        // Установка позиции в конец файла
        desc->flags = F_APPEND;
        desc->size_bytes = entry.DIR_FileSize;
        status = seek_file_fat32(desc, desc->size_bytes, F_SEEK_SET);
    }
    if (status == 0)
    {
        return desc;
    }

cleanup:
    if (desc != NULL)
    {
        if (fat32_free(desc, sizeof(FAT32_File)) != 0)
        {
            // Вывод в лог
        }
    }
    return NULL;
}

int flush_fat32(FAT32_File *file)
{
    if (fat_info == NULL || file == NULL)
        return -1;

    FatDir_Type entry = {0};
    int status = read_directory_entry_fat32(&file->entry_pos, &entry);
    if (status != 0)
        return -2;

    // Обновляем размер файла
    entry.DIR_FileSize = file->size_bytes;

    Fat32_DateTime datetime = {0};

    if (fat_info->device->datetime)
        status = fat_info->device->datetime(&datetime);

    if (status == 0)
    {
        fat32_date_to_fat(&datetime.date, &entry.DIR_WrtDate);
        fat32_time_to_fat(&datetime.time, &entry.DIR_WrtTime);
    }

    // Перезаписываем обновлённую запись директории
    status = write_dir_entries_at(&file->entry_pos, &entry, 1);

    return (status != 0 ? -3 : 0);
}

int close_file_fat32(FAT32_File **file)
{
    if (file == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    // Запись данных на накопитель
    if (flush_fat32(*file) != 0)
        return FAT32_ERR_FLUSH_FAILED;

    int status = fat32_free(*file, sizeof(FAT32_File));
    if (status != 0)
    {
        // вывод в лог
    }
    *file = NULL;
    return 0;
}

uint32_t tell_fat32(FAT32_File *file)
{
    if (file == NULL || fat_info == NULL)
    {
        return 0;
    }
    uint32_t position = file->position.cluster_idx * fat_info->secPerClus * fat_info->bytesPerSec;
    position = position + file->position.sector_idx * fat_info->bytesPerSec + file->position.byte_offset;
    return position;
}

int read_file_fat32(FAT32_File *file, uint8_t *buffer, uint32_t size)
{
    if (file == NULL || buffer == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    if (file->flags != F_READ)
    {
        return FAT32_ERR_INVALID_FILE_MODE;
    }
    if (size == 0)
    {
        return 0;
    }

    uint32_t countRBytes = 0;
    uint32_t sector = file->position.sector_idx;
    uint8_t *buffer_local = fat32_alloc(fat_info->bytesPerSec);
    if (buffer_local == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    uint32_t next_cluster = file->position.cluster_number;
    uint32_t address = fat_info->address_region + (next_cluster - fat_info->root_cluster) * fat_info->secPerClus;
    uint32_t to_copy = 0;
    uint16_t shift = file->position.byte_offset;
    uint32_t bytes_available = 0;
    int status = 0;

    if (file->size_bytes == 0)
        return 0;
    uint32_t position = tell_fat32(file);
    if (file->size_bytes < (size + position))
    {
        size = file->size_bytes - position;
    }

    while (1)
    {
        for (; sector < fat_info->secPerClus; ++sector)
        {
            status = fat_info->device->read(buffer_local, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }

            bytes_available = fat_info->bytesPerSec - shift;
            if (size - countRBytes > bytes_available)
            {
                to_copy = bytes_available;
            }
            else
            {
                to_copy = size - countRBytes;
            }

            stm_memcpy(&buffer[countRBytes], buffer_local + shift, to_copy);
            countRBytes += to_copy;
            shift += to_copy;

            if (countRBytes >= size)
            {
                file->position.sector_idx = sector;
                file->position.byte_offset = shift % fat_info->bytesPerSec;
                status = 0;
                goto cleanup;
            }
            if (shift == fat_info->bytesPerSec)
            {
                shift = 0;
            }
        }

        sector = 0;
        status = get_next_cluster_fat32(&next_cluster);
        if (status != 0 || next_cluster == FILE_END_TABLE_FAT32)
        {
            break;
        }
        address = fat_info->address_region + (next_cluster - fat_info->root_cluster) * fat_info->secPerClus;
        file->position.cluster_idx++;
        file->position.cluster_number = next_cluster;
    }

    file->position.sector_idx = sector;
    file->position.byte_offset = shift;

cleanup:
    if (fat32_free(buffer_local, fat_info->bytesPerSec))
    {
        // вывод в лог
    }
    return (status == 0 ? countRBytes : status);
}

int write_file_fat32(FAT32_File *file, uint8_t *buffer, uint32_t length)
{
    if (file == NULL || buffer == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    if (length == 0)
    {
        return 0;
    }
    if (file->flags == F_READ)
    {
        return FAT32_ERR_INVALID_FILE_MODE;
    }

    uint32_t countWBytes = 0;
    uint32_t sector = file->position.sector_idx;

    uint8_t *buffer_local = fat32_alloc(fat_info->bytesPerSec);
    if (buffer_local == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    uint32_t next_cluster = file->position.cluster_number;
    uint32_t address = fat_info->address_region + (next_cluster - fat_info->root_cluster) * fat_info->secPerClus;
    uint32_t to_copy = 0;
    uint16_t shift = file->position.byte_offset;
    int status = 0;
    FAT32_LOG_INFO("next_cluster: %d, adderess:%d, shift:%d\r\n", next_cluster, address, shift);
    while (1)
    {
        for (; sector < fat_info->secPerClus; ++sector)
        {

            // Чтение сектора в локальный буфер
            status = fat_info->device->read(buffer_local, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }

            // Вычисление количества байт для копирования
            if (((length - countWBytes) + shift) > fat_info->bytesPerSec)
            {
                to_copy = (fat_info->bytesPerSec - shift);
            }
            else
            {
                to_copy = length - countWBytes;
            }

            stm_memcpy(buffer_local + shift, &buffer[countWBytes], to_copy);
            countWBytes += to_copy;
            shift = 0;

            // Запись сектора обратно
            status = fat_info->device->write(buffer_local, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_WRITE_FAIL;
                goto cleanup;
            }
            if (countWBytes >= length)
            {
                file->position.sector_idx = sector;
                file->position.byte_offset = to_copy;
                file->size_bytes += length;
                if (fat32_free(buffer_local, fat_info->bytesPerSec) != 0)
                {
                    // лог ошибки освобождения памяти
                }
                return countWBytes;
            }
        }

        // Переход к следующему кластеру
        sector = 0;
        status = extend_cluster_chain_if_needed(&next_cluster);
        if (status != 0)
        {
            goto cleanup;
        }
        address = fat_info->address_region + (next_cluster - fat_info->root_cluster) * fat_info->secPerClus;
        file->position.cluster_idx++;
        file->position.cluster_number = next_cluster;
    }
cleanup:
    if (fat32_free(buffer_local, fat_info->bytesPerSec) != 0)
    {
        // вывод в лог
    }
    return status;
}

/**
 * Создаёт новый файл в указанной директории FAT32.
 *
 * @param cluster_directory  Кластер директории, в которой создаётся файл.
 * @param cluster_file       Указатель, в который будет записан номер первого кластера файла.
 * @param file_name          Имя файла (может быть в формате SFN или LFN).
 *
 * @return 0 при успехе или отрицательное значение — при ошибке.
 */
int create_file_fat32(uint32_t cluster_directory, uint32_t *cluster_file, char *file_name)
{
    int status = 0;
    uint16_t length = strlen(file_name);

    if (cluster_file == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    *cluster_file = FILE_END_TABLE_FAT32;

    int entry_count = 0;
    FatDir_Type *entries = NULL;

    // выделяем память(кластер) в таблице для нового файла
    status = allocate_cluster_fat32(cluster_file);
    if (status != 0)
    {
        return FAT32_ERR_CLUSTER_ALLOC_FAIL;
    }

    if (validate_fat_sfn_file(file_name) != 0)
    {
        entry_count = ((length + MAX_SYMBOLS_ENTRY) / MAX_SYMBOLS_ENTRY) + 1;

        entries = fat32_alloc(sizeof(LDIR_Type) * entry_count);
        if (entries == NULL)
        {
            return FAT32_ERR_ALLOC_FAILED;
        }

        FatDir_Type *entry = &entries[entry_count - 1];
        memset((uint8_t *)entry, 0, sizeof(FatDir_Type));
        entry->DIR_Attr = ATTR_ARCHIVE;

        split_cluster_number(*cluster_file, &entry->DIR_FstClusHI, &entry->DIR_FstClusLO);
        fat32_generate_sfn_from_lfn(file_name, length, entry->DIR_Name);
        uint8_t chksum = fat32_sfn_checksum(entry->DIR_Name);
        make_lfn_entries(file_name, length, chksum, (LDIR_Type *)entries, entry_count - 1);
    }
    else
    {
        entry_count = 1;
        entries = fat32_alloc(sizeof(FatDir_Type));
        if (entries == NULL)
        {
            return FAT32_ERR_ALLOC_FAILED;
        }
        // Ищем и добавляем запись в родительскую директорию
        FatDir_Type *entry = &entries[0];
        memset((uint8_t *)entry, 0, sizeof(FatDir_Type));

        entry->DIR_Attr = ATTR_ARCHIVE;
        split_cluster_number(*cluster_file, &entry->DIR_FstClusHI, &entry->DIR_FstClusLO);
        fat32_format_sfn(file_name, length, (char *)entry->DIR_Name);
    }

    // Найти позицию в директории с нужным количеством свободных записей
    DirEntryPosition position;
    status = find_free_dir_entries(cluster_directory, entry_count, &position);
    if (status != 0)
    {
        status = FAT32_ERR_NO_FREE_ENTRIES;
        goto cleanup;
    }
    // Записать в корневую директорию
    status = write_dir_entries_at(&position, entries, entry_count);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

cleanup:
    if (fat32_free(entries, sizeof(LDIR_Type) * entry_count) != 0)
    {
        // вывод в лог
    }
    return status;
}

int open_file_fat32(char *path, FAT32_File **file, uint8_t mode)
{
    uint32_t cluster_parent = 0;
    uint32_t file_cluster;
    int status = 0;

    if (path == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    // Проверка валидности пути
    status = validate_path(path);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_PATH;
    }

    // Получаем имя последнего сегмента
    char file_name[256];
    status = get_last_path_component(path, file_name);
    if (status != 0)
    {
        return status;
    }

    // Проверка имени файла на валидность по LFN
    status = validate_fat_lfn_file(file_name);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_CHAR;
    }

    uint32_t size = strlen(path) + 1;
    char *parent_dir_path = fat32_alloc(size);
    if (parent_dir_path == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    // Извлечение пути родительской директории
    status = get_dir_path(path, parent_dir_path, size);
    if (status != 0)
    {
        goto cleanup;
    }

    status = find_directory_fat32(path, &file_cluster);
    if (status == 0)
    {
        status = find_directory_fat32(parent_dir_path, &cluster_parent);
        if (status != 0)
        {
            goto cleanup;
        }
        // загрузить данные в дескриптор
        *file = init_file_handle(file_name, cluster_parent, mode);
        if (file == NULL)
        {
            status = FAT32_ERR_OPEN_FAILED;
        }
        else
        {
            status = 0;
        }
        goto cleanup;
    }

    status = find_directory_fat32(parent_dir_path, &cluster_parent);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    status = create_file_fat32(cluster_parent, &file_cluster, file_name);
    if (status != 0)
    {
        status = FAT32_ERR_CREATE_FAILED;
        goto cleanup;
    }

    *file = init_file_handle(file_name, cluster_parent, mode);
    if (*file != NULL)
    {
        status = 0;
    }
    else
    {
        status = FAT32_ERR_OPEN_FAILED;
    }
cleanup:
    if (fat32_free(parent_dir_path, size) != 0)
    {
        // Вывод в лог
    }
    return status;
}

int mkdir_fat32(char *path)
{
    if (path == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;

    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    // Проверка корректности формата пути
    uint32_t cluster_dir = 0;
    int status = validate_path(path);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_PATH;
    }

    // Получаем имя новой директории
    char file_name[MAX_NAME_SIZE];
    memset(file_name, '0', MAX_NAME_SIZE);
    status = get_last_path_component(path, file_name);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_PATH;
    }
    FAT32_LOG_INFO("name new dir: %s\r\n", file_name);
    // Проверка имени директории
    status = validate_fat_lfn_dir(file_name);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_CHAR;
    }

    uint32_t size = strlen(path) + 1;
    char *dir_path = fat32_alloc(size);
    if (dir_path == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    // Получаем путь к родительской директории
    status = get_dir_path(path, dir_path, size);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    if (path_exists_fat32(path) == 0)
    {
        status = 0;
        goto cleanup;
    }

    status = find_directory_fat32(dir_path, &cluster_dir);
    if (status != 0)
    {
        status = FAT32_ERR_DIR_NOT_FOUND;
        goto cleanup;
    }
    status = create_dir_fat32(file_name, strlen(file_name), cluster_dir);
    if (status != 0)
    {
        status = FAT32_ERR_CREATE_FAILED;
    }
cleanup:
    if (fat32_free(dir_path, size) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * Разделяет 32-битный номер кластера на две 16-битные части: старшую и младшую.
 *
 * Это необходимо при работе с файловой системой FAT32, где номер кластера
 * хранится в двух полях: DIR_FstClusHI (старшие 16 бит) и DIR_FstClusLO (младшие 16 бит).
 *
 * @param cluster  Полный 32-битный номер кластера.
 * @param[out] high Указатель на переменную, куда будет записана старшая 16-битная часть.
 * @param[out] low  Указатель на переменную, куда будет записана младшая 16-битная часть.
 */
void split_cluster_number(uint32_t cluster, uint16_t *high, uint16_t *low)
{
    if (low == NULL || high == NULL)
        return;
    *low = cluster & 0xFFFF;
    *high = (cluster >> 16) & 0xFFFF;
}

/**
 * Объединяет два 16-битных значения (старшую и младшую части) в одно 32-битное значение кластера.
 *
 * Эта функция используется в FAT32 для объединения двух полей из записи директории:
 * - DIR_FstClusHI (старшие 16 бит номера кластера)
 * - DIR_FstClusLO (младшие 16 бит номера кластера)
 *
 * @param cluster Указатель на переменную, в которую будет записан полный номер кластера.
 * @param high    Старшие 16 бит номера кластера.
 * @param low     Младшие 16 бит номера кластера.
 */
void join_cluster_number(uint32_t *cluster, uint16_t high, uint16_t low)
{
    if (cluster == NULL)
        return;
    *cluster = ((uint32_t)high << 16) | ((uint32_t)low << 0);
}

/**
 * Поиск свободного кластера в FAT-таблице.
 *
 * Эта функция сканирует FAT-таблицу, читая её сектора по одному, и ищет первый свободный кластер,
 * помеченный как FREE_CLUSTER.
 *
 * @param[out] free_cluster Указатель на переменную, в которую будет записан номер найденного свободного кластера.
 *                          В случае ошибки или если свободный кластер не найден, значение будет FILE_END_TABLE_FAT32.
 *
 * @return 0 при успешном поиске,
 *         FAT32_ERR_INVALID_ARGUMENT, если передан NULL-указатель,
 *         FAT32_ERR_FS_NOT_LOADED, если файловая система не инициализирована,
 *         FAT32_ERR_ALLOC_FAILED, если не удалось выделить буфер,
 *         FAT32_ERR_READ_FAIL, если не удалось прочитать сектор FAT,
 *         либо ненулевое значение в случае других ошибок.
 */
int find_free_cluster(uint32_t *free_cluster)
{
    int status = 0;
    if (free_cluster == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    *free_cluster = FILE_END_TABLE_FAT32;

    if (fat_info == NULL)
        return FAT32_ERR_FS_NOT_LOADED;

    uint32_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    int sector = 0, idx = 0;

    do
    {
        status = fat_info->device->read((uint8_t *)buffer, 1, fat_info->address_tabl1 + sector, fat_info->bytesPerSec);
        if (status < 0)
        {
            return FAT32_ERR_READ_FAIL;
        }
        for (idx = 0; idx < fat_info->bytesPerSec; ++idx)
        {
            if (buffer[idx] == FREE_CLUSTER)
            {
                *free_cluster = sector * fat_info->fat_ents_sec + idx;
                status = 0;
                goto cleanup;
            }
        }
    } while (++sector < fat_info->sizeFAT);

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // вывод в лог
    }
    return status;
}

/**
 * Обновляет запись в таблице FAT32 для указанного кластера в обеих копиях FAT.
 *
 * Функция сначала изменяет FAT1, затем FAT2. Если обновление второй копии (FAT2) не удалось,
 * FAT1 останется обновлённой, что приведёт к частичной неконсистентности.
 * В случае ошибки пользователь обязан сам обработать откат снаружи.
 *
 * @param cluster Номер кластера для обновления.
 * @param value Значение, которое будет записано (например, номер следующего кластера или EOF).
 *
 * @return 0 — успех,
 *         FAT32_ERR_INVALID_ARGUMENT — некорректный указатель fat_info,
 *         FAT32_ERR_ALLOC_FAILED — не удалось выделить память из пула,
 *         FAT32_ERR_UPDATE_FAILED — ошибка при работе с FAT1,
 *         FAT32_ERR_UPDATE_PARTIAL_FAIL — FAT1 обновлена, FAT2 — нет (требуется откат на стороне вызывающего кода).
 */
int update_fat32(uint32_t cluster, uint32_t value)
{
    if (fat_info == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;

    uint32_t sector = ((uint32_t)cluster / fat_info->fat_ents_sec);
    uint32_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    int status = 0;

    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    status = fat_info->device->read((uint8_t *)buffer, 1, fat_info->address_tabl1 + sector, fat_info->bytesPerSec);
    if (status < 0)
    {
        status = FAT32_ERR_UPDATE_FAILED;
        goto update_failed;
    }
    uint32_t idx_entry = (uint32_t)(cluster % fat_info->fat_ents_sec);

    buffer[idx_entry] = value;
    status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl1 + sector, fat_info->bytesPerSec);
    if (status < 0)
    {
        status = FAT32_ERR_UPDATE_FAILED;
        goto update_failed;
    }

    status = fat_info->device->read((uint8_t *)buffer, 1, fat_info->address_tabl2 + sector, fat_info->bytesPerSec);
    if (status < 0)
    {
        fat32_free(buffer, fat_info->bytesPerSec);
        return FAT32_ERR_UPDATE_PARTIAL_FAIL;
    }

    buffer[idx_entry] = value;
    status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl2 + sector, fat_info->bytesPerSec);
    if (status < 0)
    {
        fat32_free(buffer, fat_info->bytesPerSec);
        return FAT32_ERR_UPDATE_PARTIAL_FAIL;
    }

update_failed:
    if (status == 0)
    {
    }
    status = fat32_free((uint8_t *)buffer, fat_info->bytesPerSec);
    if (status != 0)
    {
        // вывод в лог
    }

    return status;
}

/**
 * Получает следующий кластер в цепочке FAT32.
 *
 * @param prev_cluster [in, out] - указатель на текущий кластер; при успешном завершении
 *                                будет обновлен на следующий кластер.
 * @return 0 при успехе,
 *         FAT32_ERR_INVALID_ARGUMENT если fat_info не инициализирован,
 *         FAT32_ERR_ALLOC_FAILED при ошибке выделения памяти.
 */
int get_next_cluster_fat32(uint32_t *prev_cluster)
{
    if (fat_info == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    int status = 0;
    uint32_t address = fat_info->address_tabl1 + *prev_cluster / fat_info->fat_ents_sec;
    uint32_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    status = fat_info->device->read((uint8_t *)buffer, 1, address, fat_info->bytesPerSec);
    if (status < 0)
    {
        status = FAT32_ERR_READ_FAIL;
        goto cleanup;
    }
    uint32_t idx_entry = (uint32_t)(*prev_cluster % fat_info->fat_ents_sec);
    *prev_cluster = buffer[idx_entry];

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // вывод в Лог
    }
    return status;
}

/**
 * Выделяет свободный кластер и помечает его концом цепочки (EOF) в таблице FAT.
 *
 * @param new_cluster Указатель, по которому будет записан номер выделенного кластера.
 * @return 0 при успехе,
 *         FAT32_ERR_INVALID_ARGUMENT — если аргумент NULL,
 *         FAT32_ERR_DISK_FULL — если свободный кластер не найден,
 *         FAT32_ERR_UPDATE_FAILED — если не удалось записать в обе FAT-таблицы,
 *         FAT32_ERR_UPDATE_PARTIAL_FAIL — если запись прошла только в одну таблицу, но откат удался,
 *         FAT32_ERR_RECOVERY_FAILED — если не удалось выполнить откат после частичной записи.
 */
int allocate_cluster_fat32(uint32_t *new_cluster)
{
    int status = 0;
    if (new_cluster == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    // Поиск первого свободного кластера
    status = find_free_cluster(new_cluster);
    if (status != 0)
    {
        return status;
    }
    else if (*new_cluster == FILE_END_TABLE_FAT32)
    {
        return FAT32_ERR_DISK_FULL;
    }

    // Обновляем информацию в таблицах FAT
    status = update_fat32(*new_cluster, FILE_END_TABLE_FAT32);
    if (status == FAT32_ERR_UPDATE_PARTIAL_FAIL)
    {
        status = update_fat32(*new_cluster, NOT_USED_CLUSTER_FAT32);
        if (status == FAT32_ERR_UPDATE_FAILED)
        {
            return FAT32_ERR_UPDATE_PARTIAL_FAIL;
        }
        else
        {
            status = FAT32_ERR_UPDATE_FAILED;
        }
    }
    return (status == 0 ? 0 : FAT32_ERR_UPDATE_FAILED);
}

/**
 * Проверяет, существует ли следующий кластер в цепочке,
 * и при необходимости выделяет новый кластер, расширяя цепочку.
 *
 * @param last_cluster Указатель на текущий (последний) кластер в цепочке.
 *                     При успешном расширении обновляется на следующий кластер.
 *
 * @return 0 при успехе (либо если кластер уже продолжен, либо если выделен новый),
 *         FAT32_ERR_INVALID_ARGUMENT — если аргумент некорректен,
 *         FAT32_ERR_DISK_FULL — если не удалось найти свободный кластер,
 *         FAT32_ERR_UPDATE_FAILED — ошибка обновления FAT,
 *         FAT32_ERR_UPDATE_PARTIAL_FAIL — если FAT обновлена частично,
 *         FAT32_ERR_RECOVERY_FAILED — если откат не удался.
 */
int extend_cluster_chain_if_needed(uint32_t *last_cluster)
{
    uint32_t cluster_next = *last_cluster;
    if (last_cluster == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    // Ищем следующий кластер в цепочке
    int status = get_next_cluster_fat32(&cluster_next);
    if (status == 0 && cluster_next != FILE_END_TABLE_FAT32)
    {
        *last_cluster = cluster_next;
        return 0;
    }

    // Если кластер не найден, добавляем новый кластер
    cluster_next = *last_cluster;
    status = allocate_cluster_fat32(&cluster_next);
    if (status == 0)
    {
        *last_cluster = cluster_next;
        return 0;
    }
    return status;
}

/**
 * Проверяет, является ли запись в каталоге FAT32 свободной.
 *
 * @param entry Указатель на структуру записи каталога FAT32 (FatDir_Type).
 * @return 0 — если запись свободна,
 *         1 — если запись занята,
 *        FAT32_ERR_INVALID_ARGUMENT — если передан NULL-указатель.
 *
 * Свободные записи определяются по первому байту имени:
 *   - 0x00 (ENTRY_FREE_FAT32) — запись никогда не использовалась.
 *   - 0xE5 (ENTRY_FREE_FULL_FAT32) — запись удалена.
 */
int is_free_entry_fat32(FatDir_Type *entry)
{
    if (entry == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;

    if (entry->DIR_Name[0] == ENTRY_FREE_FAT32 || entry->DIR_Name[0] == ENTRY_FREE_FULL_FAT32)
    {
        return 0;
    }
    return 1;
}

int path_exists_fat32(char *path)
{
    uint32_t cluster = 0;
    if (path == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    return (find_directory_fat32(path, &cluster) == 0 ? 0 : 1);
}

/**
 * @brief Заполняет поля имени в LFN-структуре (Long File Name) символами UTF-16.
 *
 * FAT LFN-записи хранят до 13 символов UTF-16:
 *
 * Эта функция копирует заданные символы в соответствующие поля структуры.
 * Если длина меньше 13, оставшиеся позиции заполняются 0xFFFF (указание на конец строки).
 *
 * @param entry        Указатель на структуру LDIR_Type, которую нужно заполнить.
 * @param utf16_chunk  Указатель на массив UTF-16 символов (не более 13).
 * @param length       Количество символов в utf16_chunk (от 0 до 13).
 */
void fill_lfn_name_fields(LDIR_Type *entry, const uint16_t *utf16_chunk, uint8_t length)
{
    int idx = 0;
    uint16_t *lfn_fields[MAX_SYMBOLS_ENTRY] = {0};

    for (idx = 0; idx < 5; ++idx)
        lfn_fields[idx] = (uint16_t *)&entry->LDIR_Name1[idx * 2];
    for (idx = 0; idx < 6; ++idx)
        lfn_fields[idx + 5] = (uint16_t *)&entry->LDIR_Name2[idx * 2];
    for (idx = 0; idx < 2; ++idx)
        lfn_fields[idx + 11] = (uint16_t *)&entry->LDIR_Name3[idx * 2];

    for (idx = 0; idx < MAX_SYMBOLS_ENTRY; ++idx)
    {
        if (idx < length)
        {
            *lfn_fields[idx] = utf16_chunk[idx];
        }
        else
        {
            *lfn_fields[idx] = 0xFFFF;
        }
    }
}

/**
 * @brief Создаёт LFN-записи (Long File Name) для длинного имени файла.
 *
 * FAT LFN хранит длинные имена в нескольких 13-символьных записях,
 * каждая из которых является структурой LDIR_Type.
 *
 * @param name          Исходное имя файла в ASCII.
 * @param length        Длина имени файла (в байтах).
 * @param chkSum        Контрольная сумма короткого имени (SFN), используемая в LFN-записях.
 * @param entries       Указатель на массив LDIR_Type для записи LFN-записей.
 * @param numb_entries  Количество LFN-записей (обычно вычисляется как ceil(length/13)).
 *
 * @return 0 при успехе, код ошибки при неудаче.
 */
int make_lfn_entries(const char *name, uint32_t length, uint8_t chkSum, LDIR_Type *entries, uint16_t numb_entries)
{
    if (entries == NULL || name == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;

    int idx = 0;
    uint32_t size_buffer = length + 1;
    uint16_t *name_utf16le = fat32_alloc(size_buffer * sizeof(uint16_t));
    if (name_utf16le == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    LDIR_Type *entry = NULL;

    fat32_ascii_to_utf16le(name, (uint8_t *)name_utf16le);

    for (idx = 0; idx < numb_entries; ++idx)
    {
        entry = &entries[numb_entries - idx - 1];
        entry->LDIR_Attr = ATTR_LONG_NAME;
        entry->LDIR_FstClusLO = 0;
        entry->LDIR_Type = 0;
        entry->LDIR_Chksum = chkSum;
        entry->LDIR_Ord = idx + 1;
        fill_lfn_name_fields(entry, name_utf16le + (idx * LFN_NAME_LENGTH), size_buffer - (idx * LFN_NAME_LENGTH));
    }

    // Помечаем первую (последнюю в физическом порядке) LFN-запись как последнюю (бит 0x40 в LDIR_Ord)
    entries[0].LDIR_Ord |= LFN_ENTRY_LAST;

    if (fat32_free(name_utf16le, size_buffer * sizeof(uint16_t)))
    {
        // вывод в лог
    }
    return 0;
}

/**
 * @brief Ищет свободную последовательность записей в каталоге для размещения файла или LFN-записей.
 *
 * Функция сканирует кластер(ы) каталога, чтобы найти подряд идущие `entry_count` свободных записей (32-байтных),
 * необходимых для размещения длинного имени файла (LFN) + основной SFN-записи.
 * Если свободных записей нет — цепочка кластеров расширяется.
 *
 * @param parent_cluster Кластер родительского каталога.
 * @param entry_count    Количество необходимых подряд идущих записей (обычно 1 + кол-во LFN записей).
 * @param position       Указатель на структуру, куда будет записано положение свободных записей.
 *
 * @return 0 при успехе, либо код ошибки (например, FAT32_ERR_DISK_FULL).
 */
int find_free_dir_entries(uint32_t parent_cluster, const uint16_t entry_count, DirEntryPosition *position)
{
    if (position == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    uint32_t address = 0;
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    uint32_t free_sectors = 0;
    int status = 1;
    int sector, idx;
    while (1)
    {
        free_sectors = 0;
        address = (parent_cluster - fat_info->root_cluster) * fat_info->secPerClus + fat_info->address_region;
        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            status = fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (idx = 0; idx < fat_info->bytesPerSec; idx += sizeof(LDIR_Type))
            {
                if (is_free_entry_fat32((FatDir_Type *)&buffer[idx]) == 0)
                {
                    if (++free_sectors == entry_count)
                    {
                        position->cluster = parent_cluster;
                        uint16_t count_entries_sec = (fat_info->bytesPerSec / sizeof(LDIR_Type));
                        uint16_t current_entry = sector * count_entries_sec + (idx / sizeof(LDIR_Type));
                        position->sector = (current_entry - entry_count) / (count_entries_sec);
                        position->offset = (current_entry + 1) - entry_count;
                        status = 0;
                        goto cleanup;
                    }
                }
                else
                {
                    free_sectors = 0;
                }
            }
        }
        free_sectors = 0;
        status = extend_cluster_chain_if_needed(&parent_cluster);
        if (status != 0)
        {
            // Если не осталось свободных кластеров, тогда очищаем запись для новый кластера
            update_fat32(parent_cluster, FREE_CLUSTER);
            status = FAT32_ERR_DISK_FULL;
            goto cleanup;
        }
    }
cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * @brief Записывает записи каталога (LFN + SFN) начиная с заданной позиции.
 *
 * @param position     Позиция в каталоге, куда нужно записывать.
 * @param entries      Указатель на массив записей (структуры LDIR_Type / FatDir_Type).
 * @param entry_count  Количество записей для записи.
 *
 * @return 0 при успехе, иначе код ошибки.
 */
int write_dir_entries_at(const DirEntryPosition *position, const void *entries, uint16_t entry_count)
{
    if (position == NULL || entries == NULL)
        return FAT32_ERR_INVALID_ARGUMENT;
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    memset(buffer, 0, fat_info->bytesPerSec);
    uint16_t entries_written = 0;
    uint32_t address = (position->cluster - fat_info->root_cluster) * fat_info->secPerClus + fat_info->address_region;
    int status = 0;

    uint16_t to_copy = 0;
    int sector = position->sector;

    uint16_t count_entries_sector = fat_info->bytesPerSec / sizeof(LDIR_Type);
    if (position->offset != 0)
    {
        if (fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        };

        if (entry_count > (count_entries_sector - position->offset))
        {
            to_copy = entry_count - (count_entries_sector - position->offset);
        }
        else
        {
            to_copy = entry_count;
        }

        stm_memcpy(buffer + position->offset * sizeof(LDIR_Type), (uint8_t *)entries, to_copy * sizeof(LDIR_Type));
        if (fat_info->device->write(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
        {
            status = FAT32_ERR_WRITE_FAIL;
            goto cleanup;
        }
        ++sector;
        entries_written += to_copy;
    }

    for (; sector < fat_info->secPerClus && entries_written < entry_count; ++sector)
    {
        if (fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        };
        to_copy = count_entries_sector;
        if (entries_written + to_copy > entry_count)
        {
            to_copy = entry_count - entries_written;
        }
        stm_memcpy(buffer, (const uint8_t *)(entries + entries_written), to_copy * sizeof(LDIR_Type));
        if (fat_info->device->write(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
        {
            status = FAT32_ERR_WRITE_FAIL;
            goto cleanup;
        };
        entries_written += to_copy;
    }
cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * @brief Создаёт новую директорию в FAT32.
 *
 * @param name            Имя директории (LFN или SFN).
 * @param length          Длина имени.
 * @param parent_cluster  Кластер родительской директории.
 * @return 0 при успехе, иначе код ошибки.
 */
int create_dir_fat32(char *name, uint32_t length, uint32_t parent_cluster)
{
    int status = 0;
    uint32_t cluster_new_dir = 0;

    int entry_count = 0;
    FatDir_Type *entries = NULL;

    if (name == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    // выделяем память в таблице для новой директории
    status = allocate_cluster_fat32(&cluster_new_dir);
    if (status != 0)
    {
        return FAT32_ERR_CLUSTER_ALLOC_FAIL;
    }
    FatDir_Type *entry = NULL;
    if (validate_fat_sfn_dir(name) == 0)
    {
        entry_count = ((length + MAX_SYMBOLS_ENTRY) / MAX_SYMBOLS_ENTRY) + 1;

        entries = fat32_alloc(sizeof(LDIR_Type) * entry_count);
        if (entries == NULL)
        {
            return FAT32_ERR_ALLOC_FAILED;
        }
        entry = &entries[entry_count - 1];
        memset((uint8_t *)entry, 0, sizeof(FatDir_Type));

        split_cluster_number(cluster_new_dir, &entry->DIR_FstClusHI, &entry->DIR_FstClusLO);
        fat32_generate_sfn_from_lfn(name, length, entry->DIR_Name);
        uint8_t chksum = fat32_sfn_checksum(entry->DIR_Name);
        make_lfn_entries(name, length, chksum, (LDIR_Type *)entries, entry_count - 1);
    }
    else
    {
        entry_count = 1;
        entries = fat32_alloc(sizeof(FatDir_Type));
        if (entries == NULL)
        {
            return FAT32_ERR_ALLOC_FAILED;
        }
        // Ищем и добавляем запись в родительскую директорию
        entry = &entries[0];
        memset((uint8_t *)entry, 0, sizeof(FatDir_Type));

        split_cluster_number(cluster_new_dir, &entry->DIR_FstClusHI, &entry->DIR_FstClusLO);
        fat32_format_sfn(name, length, (char *)entry->DIR_Name);
    }

    Fat32_DateTime datetime = {0};

    if (fat_info->device->datetime)
        status = fat_info->device->datetime(&datetime);

    entry->DIR_Attr = ATTR_DIRECTORY;
    if (status == 0)
    {
        fat32_date_to_fat(&datetime.date, &entry->DIR_CrtDate);
        fat32_time_to_fat(&datetime.time, &entry->DIR_CrtTime);
        entry->DIR_CrtTimeTenth = 0;
        entry->DIR_WrtDate = entry->DIR_CrtDate;
        entry->DIR_WrtTime = entry->DIR_CrtTime;
        entry->DIR_LstAccDate = entry->DIR_CrtDate;
    }

    // Поиск свободного места в директории
    DirEntryPosition position;
    status = find_free_dir_entries(parent_cluster, entry_count, &position);
    if (status != 0)
    {
        status = FAT32_ERR_NO_FREE_ENTRIES;
        goto cleanup;
    }
    // Записать в корневую директорию
    status = write_dir_entries_at(&position, entries, entry_count);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

    // Создаем entries для новой папки

    FatDir_Type dir_entries[2];
    memset((uint8_t *)dir_entries, 0, sizeof(FatDir_Type) * 2);

    dir_entries[0].DIR_Attr = dir_entries[1].DIR_Attr = ATTR_DIRECTORY;
    dir_entries[0].DIR_FileSize = dir_entries[1].DIR_FileSize = 0;

    memset(dir_entries[0].DIR_Name, ' ', sizeof(dir_entries[0].DIR_Name));
    memset(dir_entries[1].DIR_Name, ' ', sizeof(dir_entries[0].DIR_Name));
    stm_memcpy(dir_entries[0].DIR_Name, (uint8_t *)"..", 2);
    stm_memcpy(dir_entries[1].DIR_Name, (uint8_t *)".", 1);

    split_cluster_number(position.cluster, &dir_entries[0].DIR_FstClusHI, &dir_entries[0].DIR_FstClusLO);
    split_cluster_number(cluster_new_dir, &dir_entries[1].DIR_FstClusHI, &dir_entries[1].DIR_FstClusLO);

    position.cluster = cluster_new_dir;
    position.offset = 0;
    position.sector = 0;

    status = write_dir_entries_at(&position, dir_entries, 2);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

cleanup:
    if (fat32_free(entries, sizeof(LDIR_Type) * entry_count) != 0)
    {
        // Вывод в лог
    }

    return status;
}

/**
 * @brief Удаляет цепочку кластеров, связанных с файлом или директорией в FAT32.
 *
 * Освобождает все кластеры, начиная с переданного `cluster_file`, путём последовательного
 * чтения следующего кластера через FAT и пометки текущего как свободного (FREE_CLUSTER).
 *
 * @param cluster_file Начальный кластер файла или директории, которую требуется удалить.
 * @return int Код ошибки или 0 при успешном завершении:
 *  - FAT32_ERR_FS_NOT_LOADED — если файловая система не инициализирована.
 *  - FAT32_ERR_INVALID_CLUSTER — если передан некорректный номер кластера.
 *  - FAT32_ERR_READ_FAIL — если не удалось прочитать следующий кластер.
 *  - FAT32_ERR_WRITE_FAIL — если не удалось обновить FAT.
 *  - FAT32_ERR_INVALID_CLUSTER_CHAIN — если цепочка повреждена (бесконечный цикл).
 */
int delete_entry_fat32(uint32_t cluster_file)
{
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    if (cluster_file < 2 || cluster_file >= FILE_END_TABLE_FAT32)
    {
        return FAT32_ERR_INVALID_CLUSTER;
    }
    uint32_t next_cluster = cluster_file;
    int status = 0;
    while (cluster_file < FILE_END_TABLE_FAT32)
    {
        status = get_next_cluster_fat32(&next_cluster);
        if (status != 0)
        {
            return FAT32_ERR_READ_FAIL;
        }

        status = update_fat32(cluster_file, FREE_CLUSTER);
        if (status != 0)
        {
            return FAT32_ERR_WRITE_FAIL;
        }
        if (cluster_file == next_cluster)
        {
            return FAT32_ERR_INVALID_CLUSTER_CHAIN;
        }

        cluster_file = next_cluster;
    };
    return 0;
}

/**
 * @brief Рекурсивно удаляет директорию и всё её содержимое (файлы и поддиректории) в FAT32.
 *
 * @param cluster Кластер директории, которую необходимо удалить.
 * @return int 0 при успехе, либо отрицательный код ошибки:
 *  - FAT32_ERR_FS_NOT_LOADED — если файловая система не инициализирована.
 *  - FAT32_ERR_ALLOC_FAILED — ошибка при выделении буфера.
 *  - FAT32_ERR_READ_FAIL — ошибка чтения сектора.
 *  - FAT32_ERR_WRITE_FAIL — ошибка записи сектора.
 *  - FAT32_ERR_DELETE_PROTECTED — попытка удалить системный файл.
 */
int delete_dir_recursive_fat32(uint32_t cluster)
{
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    FatDir_Type *entry_child;

    uint32_t cluster_child;

    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    uint32_t sector = 0;
    uint32_t address = 0;
    uint32_t idx = 0;
    int status = 0;

    while (1)
    {
        address = fat_info->address_region + (cluster - fat_info->root_cluster) * fat_info->secPerClus;

        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            if (fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (idx = 0; idx < fat_info->bytesPerSec; idx += sizeof(FatDir_Type *))
            {
                entry_child = (FatDir_Type *)&buffer[idx];
                if (entry_child->DIR_Name[0] == ENTRY_FREE_FULL_FAT32)
                {
                    status = 1;
                    break;
                }
                else if (entry_child->DIR_Name[0] == ENTRY_FREE_FAT32)
                {
                    continue;
                }
                if ((entry_child->DIR_Attr & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME)
                {
                    join_cluster_number(&cluster_child, entry_child->DIR_FstClusHI, entry_child->DIR_FstClusLO);

                    if (fat32_is_special_dir(entry_child->DIR_Name) == 0)
                    {
                        memset(entry_child, 0x00, sizeof(FatDir_Type));
                        continue;
                    }

                    if (entry_child->DIR_Attr & ATTR_DIRECTORY)
                    {
                        status = delete_dir_recursive_fat32(cluster_child);
                        if (status != 0)
                        {
                            goto cleanup;
                        };
                    }
                    else if (entry_child->DIR_Attr == ATTR_SYSTEM)
                    {
                        status = FAT32_ERR_DELETE_PROTECTED;
                        goto cleanup;
                    }
                    status = delete_entry_fat32(cluster_child);
                    if (status != 0)
                    {
                        goto cleanup;
                    };
                }
                entry_child->DIR_Name[0] = ENTRY_FREE_FAT32;
            }
            if (fat_info->device->write(buffer, 1, address, fat_info->bytesPerSec) != 0)
            {
                status = FAT32_ERR_WRITE_FAIL;
                goto cleanup;
            }
            if (status == 1)
            {
                status = 0;
                goto cleanup;
            }
        }
        if (get_next_cluster_fat32(&cluster) != 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        }
        if (cluster == FILE_END_TABLE_FAT32)
        {
            break;
        }
    }

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * @brief Проверяет, пустая ли директория (не содержит файлов и папок, кроме "." и "..").
 *
 * @param cluster Кластер директории для проверки.
 * @return int
 *   1 — директория пустая,
 *   0 — содержит записи,
 *   <0 — код ошибки.
 */
int is_dir_empty_fat32(uint32_t cluster)
{
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    int sector = 0;
    uint32_t address = 0;
    uint32_t idx = 2 * sizeof(FatDir_Type); // // Пропускаем первые две записи "." и ".."
    int status = 0;
    while (cluster != FILE_END_TABLE_FAT32)
    {
        address = fat_info->address_region + (cluster - fat_info->root_cluster) * fat_info->secPerClus;
        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            if (fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec) < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (; idx < fat_info->bytesPerSec; idx += sizeof(FatDir_Type))
            {
                if (buffer[idx] == ENTRY_FREE_FULL_FAT32)
                {
                    return 0;
                }
                if (buffer[idx] == ENTRY_FREE_FAT32)
                {
                    continue;
                }
                else
                {
                    status = FAT32_ERR_DIR_NOT_EMPTY;
                    goto cleanup;
                }
            }
            idx = 0;
        }
        status = get_next_cluster_fat32(&cluster);
        if (status != 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        }
    }
cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }
    return status;
}

/**
 * @brief Получить атрибут (DIR_Attr) записи в каталоге по её кластеру
 *
 * Функция ищет в родительском каталоге (`cluster_parent`) запись, которая
 * указывает на `child_cluster`, и возвращает её атрибуты (`DIR_Attr`).
 *
 * @param cluster_parent Кластер родительского каталога
 * @param child_cluster Кластер дочернего элемента (файла/каталога)
 * @param attr [out] Указатель на переменную, в которую будет записан атрибут
 * @return int Код возврата:
 *         0 — успех,
 *         < 0 — код ошибки (например, FAT32_ERR_FS_NOT_LOADED, FAT32_ERR_READ_FAIL, FAT32_ERR_ENTRY_NOT_FOUND)
 */
int get_attr_entry_fat32(uint32_t cluster_parent, uint32_t child_cluster, uint8_t *attr)
{
    if (attr == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    int sector = 0;
    uint32_t address = 0;
    uint32_t idx = 0;
    int status = 0;
    uint32_t cluster = 0;
    FatDir_Type *entry = NULL;

    while (1)
    {
        address = fat_info->address_region + (cluster_parent - fat_info->root_cluster) * fat_info->secPerClus;
        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            if (fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec) != 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (idx = 0; idx < fat_info->bytesPerSec; idx += sizeof(FatDir_Type))
            {
                if (buffer[idx] == ENTRY_FREE_FULL_FAT32)
                {
                    status = 0;
                    goto cleanup;
                }
                if (buffer[idx] == ENTRY_FREE_FAT32)
                {
                    continue;
                }
                entry = (FatDir_Type *)&buffer[idx];
                join_cluster_number(&cluster, entry->DIR_FstClusHI, entry->DIR_FstClusLO);
                if (cluster == child_cluster)
                {
                    *attr = entry->DIR_Attr;
                    status = 0;
                    goto cleanup;
                }
            }
        }
        status = get_next_cluster_fat32(&cluster);
        if (status != 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        }
        if (cluster == FILE_END_TABLE_FAT32)
        {
            status = FAT32_ERR_ENTRY_NOT_FOUND;
            goto cleanup;
        }
    }

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // вывести в лог
    }
    return status;
}

/**
 * @brief Помечает запись каталога (и связанные с ней LFN-записи) как удалённые
 *
 * Функция ищет запись с указанным именем в родительском кластере,
 * а затем помечает как удалёнными все связанные с ней LFN-записи и саму SFN-запись.
 * Удаление производится в обратном порядке — от SFN к началу цепочки LFN.
 *
 * @param parent_cluster Кластер родительского каталога
 * @param name Имя файла/каталога, который нужно удалить
 * @return int Код возврата:
 *         0 — успешно,
 *         < 0 — код ошибки (FAT32_ERR_ENTRY_NOT_FOUND, FAT32_ERR_READ_FAIL, FAT32_ERR_WRITE_FAIL и т.д.)
 */
int mark_dir_entry_deleted(uint32_t parent_cluster, char *name)
{
    FatDir_Type *entries = fat32_alloc(fat_info->bytesPerSec / sizeof(FatDir_Type));
    if (entries == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    DirEntryPosition pos = {0};
    int status = 0;
    status = find_entry_by_name(name, parent_cluster, &pos);
    if (status != 0)
    {
        status = FAT32_ERR_ENTRY_NOT_FOUND;
        goto cleanup;
    }
    uint32_t address = fat_info->address_region + (pos.cluster - fat_info->root_cluster) * fat_info->secPerClus;

    uint32_t sector = pos.sector;
    int idx = pos.offset;
    FatDir_Type *entry = NULL;
    uint8_t found_sfn = 0;

    for (; sector >= 0; --sector)
    {
        if (fat_info->device->read((uint8_t *)entries, 1, address + sector, fat_info->bytesPerSec) != 0)
        {
            status = FAT32_ERR_READ_FAIL;
            goto cleanup;
        }
        for (; idx >= 0; --idx)
        {
            entry = &entries[idx];
            if ((entry->DIR_Attr & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME)
            {
                if (found_sfn == 1)
                {
                    status = 0;
                    break;
                }
                else
                {
                    found_sfn = 1;
                }
            }
            entry->DIR_Name[0] = ENTRY_FREE_FAT32;
        }
        if (fat_info->device->write((uint8_t *)entries, 1, address + sector, fat_info->bytesPerSec) != 0)
        {
            status = FAT32_ERR_WRITE_FAIL;
            goto cleanup;
        }
        if (status == 0)
        {
            break;
        }

        idx = fat_info->bytesPerSec / sizeof(FatDir_Type);
    }
cleanup:
    if (fat32_free(entries, fat_info->bytesPerSec / sizeof(FatDir_Type)) != 0)
    {
        // Вывести в лог
    }
    return status;
}

/**
 * @brief Удаление обычного файла по указанному пути
 *
 * Функция удаляет файл в файловой системе FAT32:
 * 1. Проверяет путь и наличие FS.
 * 2. Находит кластер файла и его родительский каталог.
 * 3. Проверяет, что объект не является системным файлом или директорией.
 * 4. Удаляет запись в каталоге.
 * 5. Освобождает все кластеры, занятые файлом.
 *
 * @param path Абсолютный путь до файла (например, "/dir1/file.txt")
 * @return int Код возврата:
 *         0 — успех,
 *         < 0 — код ошибки (например, FAT32_ERR_*)
 */
int delete_file_fat32(char *path)
{
    if (path == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }

    int status = validate_path(path);
    if (status != 0)
    {
        return FAT32_ERR_INVALID_PATH;
    }
    uint32_t file_cluster;
    status = find_directory_fat32(path, &file_cluster);
    if (status != 0)
    {
        return FAT32_ERR_ENTRY_NOT_FOUND;
    }

    uint32_t size = strlen(path) + 1;
    char *parent_dir_path = fat32_alloc(size);
    if (parent_dir_path == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    };

    status = get_dir_path(path, parent_dir_path, size);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }
    uint32_t parent_cluster = 0;
    status = find_directory_fat32(parent_dir_path, &parent_cluster);
    if (status != 0)
    {
        status = FAT32_ERR_ENTRY_NOT_FOUND;
        goto cleanup;
    }

    // проверка что это директория
    uint8_t attr = 0;
    status = get_attr_entry_fat32(parent_cluster, file_cluster, &attr);
    if (status != 0 || attr == ATTR_SYSTEM)
    {
        status = FAT32_ERR_ENTRY_NOT_FOUND;
        goto cleanup;
    }
    if (attr & ATTR_DIRECTORY || attr == ATTR_SYSTEM)
    {
        status = FAT32_ERR_IS_DIRECTORY;
        goto cleanup;
    }

    char file_name[256];
    status = get_last_path_component(path, file_name);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    status = mark_dir_entry_deleted(parent_cluster, file_name);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

    status = delete_entry_fat32(file_cluster);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }
cleanup:
    if (fat32_free(parent_dir_path, size) != 0)
    {
        // Вывести в лог
    }
    return status;
}

/**
 * @brief Удаление директории по указанному пути
 *
 * В зависимости от режима удаления:
 * - В режиме DELETE_DIR_SAFE директория удаляется только если пуста.
 * - В режиме DELETE_DIR_RECURSIVE производится рекурсивное удаление всех вложенных файлов и поддиректорий.
 *
 * @param path Абсолютный путь до директории (например, "/dir1/subdir")
 * @param mode Режим удаления:
 *             DELETE_DIR_SAFE — только если пуста;
 *             DELETE_DIR_RECURSIVE — рекурсивное удаление содержимого.
 * @return int Код возврата:
 *         0 — успех,
 *         < 0 — ошибка (например, FAT32_ERR_*)
 */
int delete_dir_fat32(char *path, DeleteDirMode mode)
{
    if (path == NULL)
    {
        return FAT32_ERR_INVALID_PATH;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    char *path_buff = NULL;
    char *parent_dir_path = NULL;

    uint32_t length = strlen(path);
    if (length > 0 && path[length - 1] == '/')
        --length;
    path_buff = fat32_alloc(length);
    if (path_buff == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    strncpy(path_buff, path, length);

    int status = validate_path(path_buff);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    uint32_t dir_cluster;
    status = find_directory_fat32(path_buff, &dir_cluster);

    if (status != 0)
    {
        status = FAT32_ERR_ENTRY_NOT_FOUND;
        goto cleanup;
    }

    parent_dir_path = fat32_alloc(length);
    if (parent_dir_path == NULL)
    {
        status = FAT32_ERR_ALLOC_FAILED;
        goto cleanup;
    };

    status = get_dir_path(path_buff, parent_dir_path, length);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }
    uint32_t parent_cluster = 0;
    status = find_directory_fat32(parent_dir_path, &parent_cluster);
    if (status != 0)
    {
        status = FAT32_ERR_ENTRY_NOT_FOUND;
        goto cleanup;
    }

    // проверка что это директория
    uint8_t attr = 0;
    status = get_attr_entry_fat32(parent_cluster, dir_cluster, &attr);
    if (status != 0 || attr != ATTR_DIRECTORY)
    {
        status = FAT32_ERR_NOT_A_DIRECTORY;
        goto cleanup;
    }

    if (mode == DELETE_DIR_SAFE)
    {
        if (is_dir_empty_fat32(dir_cluster) != 0)
        {
            status = FAT32_ERR_DIR_NOT_EMPTY;
            goto cleanup;
        }
    }
    else
    {
        status = delete_dir_recursive_fat32(dir_cluster);
        if (status != 0)
        {
            status = FAT32_ERR_DELETE_FAIL;
            goto cleanup;
        }
    }

    char file_name[256];
    status = get_last_path_component(path_buff, file_name);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }
    status = mark_dir_entry_deleted(parent_cluster, file_name);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

    status = delete_entry_fat32(dir_cluster);
    if (status != 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
    }
cleanup:
    if (path_buff != NULL)
    {
        if (fat32_free(path_buff, length) != 0)
        {
            // Вывести в лог
        }
    }
    if (parent_dir_path != NULL)
    {
        if (fat32_free(parent_dir_path, length) != 0)
        {
            // Вывести в лог
        }
    }
    return status;
}

/**
 * @brief Читает одну запись каталога FAT32 по заданной позиции
 *
 * @param position Структура с координатами (кластер, сектор, индекс записи)
 * @param entry Указатель на структуру, куда будет скопирована запись
 * @return int Код возврата:
 *         0 — успех,
 *         < 0 — ошибка (например, FAT32_ERR_*)
 */
int read_directory_entry_fat32(DirEntryPosition *position, FatDir_Type *entry)
{
    int status = 0;
    if (position == NULL || entry == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (fat_info == NULL)
    {
        return FAT32_ERR_FS_NOT_LOADED;
    }
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    uint32_t address = fat_info->address_region + (position->cluster - fat_info->root_cluster) * fat_info->secPerClus + position->sector;
    if (fat_info->device->read(buffer, 1, address, fat_info->bytesPerSec) != 0)
    {
        status = FAT32_ERR_READ_FAIL;
        goto cleanup;
    }
    stm_memcpy((uint8_t *)entry, (uint8_t *)buffer + position->offset * sizeof(FatDir_Type), sizeof(FatDir_Type));

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Добавить в лог
    }
    return status;
}

/**
 * @brief Сравнивает имя файла с коротким именем (SFN) из FAT32-структуры каталога.
 *
 * @param name Имя файла (в обычном виде, например "file.txt")
 * @param length Длина строки `name`
 * @param entry Указатель на структуру FatDir_Type (запись каталога FAT32)
 * @return int 0 если имена совпадают, -1 если нет или если имя длиннее 11 символов
 */
int fat32_compare_sfn(char *name, uint32_t length, const FatDir_Type *entry)
{
    if (name == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    if (length > SHORT_NAME_SIZE)
        return -1;
    char buffer[SHORT_NAME_SIZE];
    fat32_format_sfn(name, strlen(name), buffer);
    for (int idx = 0; idx < SHORT_NAME_SIZE; ++idx)
    {
        if (buffer[idx] != entry->DIR_Name[idx])
        {
            return -1;
        }
    }
    return 0;
}

/**
 * @brief Находит кластер записи (файла или папки) с заданным именем в каталоге FAT32.
 *
 * Функция перебирает все записи в директории, заданной кластером parent_cluster,
 * ищет запись с именем name. Имя может быть в коротком (SFN) или длинном (LFN) формате.
 *
 * @param name Имя файла или папки в ASCII.
 * @param length Длина имени.
 * @param parent_cluster Кластер каталога, в котором ищется запись.
 * @param out_cluster Указатель на переменную для записи кластера найденной записи.
 * @return 0 при успехе,
 *         FAT32_ERR_ALLOC_FAILED при ошибке выделения памяти,
 *         FAT32_ERR_NOT_FOUND если запись не найдена,
 *         или другой код ошибки при чтении.
 */
int find_entry_cluster_fat32(char *name, uint32_t length, uint32_t parent_cluster, uint32_t *out_cluster)
{
    uint8_t *buffer = fat32_alloc(fat_info->bytesPerSec);
    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    uint32_t address = (parent_cluster - fat_info->root_cluster) * fat_info->secPerClus + fat_info->address_region;
    uint32_t sector = 0, idxEntry = 0;
    int status = 0;
    uint16_t buffer_name[MAX_NAME_SIZE];

    FatDir_Type *entry;
    uint8_t lfn_active = 0;
    uint8_t chksum_exp = 0;

    uint8_t idx = 0;
    uint8_t order = 0;

    while (1)
    {
        for (sector = 0; sector < fat_info->secPerClus; ++sector)
        {
            status = fat_info->device->read(buffer, 1, address + sector, fat_info->bytesPerSec);
            if (status < 0)
            {
                status = FAT32_ERR_READ_FAIL;
                goto cleanup;
            }
            for (idxEntry = 0; idxEntry < fat_info->bytesPerSec; idxEntry += sizeof(FatDir_Type))
            {
                entry = (FatDir_Type *)&buffer[idxEntry];
                if (entry->DIR_Name[0] == ENTRY_FREE_FULL_FAT32)
                {
                    status = FAT32_ERR_NOT_FOUND;
                    goto cleanup;
                }
                else if (entry->DIR_Name[0] == ENTRY_FREE_FAT32)
                {
                    if (lfn_active)
                    {
                        status = FAT32_ERR_NOT_FOUND;
                        goto cleanup;
                    }
                    continue;
                }
                else if ((entry->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME)
                {
                    if (((LDIR_Type *)entry)->LDIR_Ord & LFN_ENTRY_LAST)
                        chksum_exp = ((LDIR_Type *)entry)->LDIR_Chksum;
                    if (((LDIR_Type *)entry)->LDIR_Type != 0)
                        continue;
                    order = ((LDIR_Type *)entry)->LDIR_Ord & ~LFN_ENTRY_LAST;

                    idx = (order - 1) * MAX_SYMBOLS_ENTRY;
                    stm_memcpy((uint8_t *)(&buffer_name[idx]), ((LDIR_Type *)entry)->LDIR_Name1, sizeof(((LDIR_Type *)entry)->LDIR_Name1));
                    idx += sizeof(((LDIR_Type *)entry)->LDIR_Name1) / 2;
                    stm_memcpy((uint8_t *)&buffer_name[idx], ((LDIR_Type *)entry)->LDIR_Name2, sizeof(((LDIR_Type *)entry)->LDIR_Name2));
                    idx += sizeof(((LDIR_Type *)entry)->LDIR_Name2) / 2;
                    stm_memcpy((uint8_t *)&buffer_name[idx], ((LDIR_Type *)entry)->LDIR_Name3, sizeof(((LDIR_Type *)entry)->LDIR_Name3));
                    lfn_active = 1;
                }
                else
                {
                    if (lfn_active)
                    {
                        uint8_t chksum_calc = fat32_sfn_checksum(entry->DIR_Name);
                        if (chksum_calc == chksum_exp)
                        {
                            if (fat32_compare_lfn(name, buffer_name) == 0)
                            {
                                join_cluster_number(out_cluster, entry->DIR_FstClusHI, entry->DIR_FstClusLO);
                                status = 0;
                                goto cleanup;
                            }
                        }
                        lfn_active = 0;
                        chksum_exp = 0;
                        memset(buffer_name, 0x00, MAX_NAME_SIZE);
                    }
                    else if (fat32_compare_sfn(name, length, (FatDir_Type *)entry) == 0)
                    {
                        join_cluster_number(out_cluster, entry->DIR_FstClusHI, entry->DIR_FstClusLO);
                        status = 0;
                        goto cleanup;
                    }
                }
            }
        }
    }
    status = 1;

cleanup:
    if (fat32_free(buffer, fat_info->bytesPerSec) != 0)
    {
        // Вывод в лог
    }

    return status;
}

/**
 * @brief Находит кластер директории по заданному пути в файловой системе FAT32.
 *
 * Функция разбивает путь на компоненты (поддиректории),
 * затем последовательно ищет каждый компонент в файловой системе,
 * начиная с корневого кластера.
 *
 * @param path Строка с путем к директории (например, "/folder/subfolder").
 * @param out_cluster Указатель на переменную, куда будет записан номер кластера найденной директории.
 * @return 0 в случае успеха, или код ошибки в случае неудачи.
 */
int find_directory_fat32(char *path, uint32_t *out_cluster)
{
    int status = 0;
    char *pathToDir = NULL;
    char (*directories)[MAX_NAME_SIZE] = NULL;
    if (path == NULL || out_cluster == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    pathToDir = fat32_alloc(strlen(path) + 1);
    if (pathToDir == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }
    stm_memcpy(pathToDir, path, strlen(path) + 1);

    if (strcmp(path, "/") == 0)
    {
        *out_cluster = fat_info->root_cluster;
        status = 0;
        goto cleanup;
    }

    status = validate_path(pathToDir);
    if (status != 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    uint32_t depth = fat32_path_depth(pathToDir);
    directories = fat32_alloc(depth * MAX_NAME_SIZE);
    if (directories == NULL)
    {
        status = FAT32_ERR_ALLOC_FAILED;
        goto cleanup;
    }

    memset(directories, 0, depth * MAX_NAME_SIZE);

    status = fat32_parse_path(pathToDir, directories);
    if (status < 0)
    {
        status = FAT32_ERR_INVALID_PATH;
        goto cleanup;
    }

    *out_cluster = fat_info->root_cluster;
    for (int idxDir = 0; idxDir < depth; ++idxDir)
    {
        status = find_entry_cluster_fat32(directories[idxDir], strlen(directories[idxDir]), *out_cluster, out_cluster);
        if (status != 0)
        {
            status = FAT32_ERR_DIR_NOT_FOUND;
            goto cleanup;
        }
    }
cleanup:
    if (pathToDir != NULL)
    {
        if (fat32_free(pathToDir, strlen(path) + 1) != 0)
        {
            // вывод в лог
        }
    }
    if (directories != NULL)
    {
        if (fat32_free(directories, depth * MAX_NAME_SIZE) != 0)
        {
            // вывод в лог
        }
    }
    return status;
}

/**
 * @brief Вычисляет общее количество секторов для FAT32 тома с учетом служебных областей.
 *
 * Эта функция рассчитывает минимальное количество секторов между
 * вычисленным размером служебных областей FAT32 (резервных секторов,
 * FAT таблиц и записей FAT) и реальным размером тома.
 * Это необходимо для корректной работы с файловой системой и предотвращения выхода за пределы.
 *
 * @param volume Общий размер тома в байтах.
 * @param mbr_data Указатель на структуру с параметрами тома из MBR (Master Boot Record).
 * @return Общее количество секторов (32-битное значение).
 */
uint32_t calc_tot_sec32(uint64_t volume, const MBR_Type *mbr_data)
{
    uint32_t record_sectors = (mbr_data->BPB_BytsPerSec / 4) * mbr_data->BPB_SecPerClus;
    uint32_t fat_sectors = mbr_data->BPB_RsvdSecCnt + mbr_data->BPB_FATSz32 * mbr_data->BPB_NumFATs + mbr_data->BPB_FATSz32 * record_sectors;
    uint32_t total_sectors = volume / mbr_data->BPB_BytsPerSec;
    return (fat_sectors > total_sectors ? total_sectors : fat_sectors);
}

int formatted_fat32(BlockDevice *device, uint64_t capacity)
{
    if (device == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }
    int status = 0;
    // Загрузить данные MBR
    MBR_Type mbr_data = {0};
    // MBR_Type mbr_data = {0};
    memset(&mbr_data, 0, sizeof(MBR_Type));
    // mbr_data->BS_jmpBoot = {0xeb, 0x58, 0x90};//можно не указывать это для перехода в область памяти для загрузки ОС
    stm_memcpy(mbr_data.BS_OEMName, "STM32_MS", sizeof(mbr_data.BS_OEMName));

    if (device->block_size == 0)
        mbr_data.BPB_BytsPerSec = 512;
    mbr_data.BPB_BytsPerSec = device->block_size;

    if (capacity >= SIZE_2GB && capacity <= SIZE_8GB)
    {
        mbr_data.BPB_SecPerClus = 8; // 4096 размер сектора / 512
    }
    else
    {
        return -1;
    }

    mbr_data.BPB_Media = 0xF8;

    mbr_data.BPB_RootEntCnt = 0;
    mbr_data.BPB_SecPerTrk = 63; // по default
    mbr_data.BPB_NumHeads = 255; // по default

    mbr_data.BPB_HiddSec = 0;

    // Настройка таблиц
    mbr_data.BPB_NumFATs = 2;
    mbr_data.BPB_RsvdSecCnt = 32;

    FAT32_LOG_INFO(
        "FAT32 MBR part1 {BPB_NumFATs: %d, BPB_SecPerClus: %d, BPB_BytsPerSec: %d}\r\n",
        mbr_data.BPB_NumFATs,
        mbr_data.BPB_SecPerClus,
        mbr_data.BPB_BytsPerSec);

    mbr_data.BPB_FATSz32 = calculate_size_table_fat32(capacity, &mbr_data);

    mbr_data.BPB_TotSec32 = calc_tot_sec32(capacity, &mbr_data);

    mbr_data.BPB_ExtFlags = 0;
    mbr_data.BPB_FSVer = 0;

    mbr_data.BPB_RootClus = 2;
    mbr_data.BPB_FSInfo = 1;
    mbr_data.BPB_BkBootSec = 6;

    mbr_data.BS_DrvNum = 0x80;
    mbr_data.BS_BootSig = 0x29;
    mbr_data.BS_VolID = 345;
    stm_memcpy(mbr_data.BS_VolLab, "STM32F407", sizeof(mbr_data.BS_VolLab));
    stm_memcpy(mbr_data.BS_FilSysType, "FAT32 ", sizeof(mbr_data.BS_FilSysType));
    mbr_data.Signature_word = WORD_SIGNATURE;

    FAT32_LOG_INFO(
        "FAT32 MBR {BPB_FATSz32: %d, BPB_TotSec32: %d, BPB_RootClus: %d}\r\n",
        mbr_data.BPB_FATSz32,
        mbr_data.BPB_TotSec32,
        mbr_data.BPB_RootClus);

    // ========== 1. Очистка системной области ==========
    // Очищаются начальные системные сектора:
    // - Главный загрузочный сектор (LBA 0)
    // - FSInfo-сектор (LBA 1)
    // - Копия загрузочного сектора (Backup Boot Sector, LBA 6)

    uint32_t sectors_to_clear = mbr_data.BPB_RsvdSecCnt +
                                mbr_data.BPB_NumFATs * mbr_data.BPB_FATSz32;

    status = device->clear(0, sectors_to_clear, mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        FAT32_LOG_INFO("Error device clear sectors [0, %d] (status: %d)!\r\n", sectors_to_clear, status);
        return FAT32_ERR_WRITE_FAIL;
    }
    FAT32_LOG_INFO("Successfully cleared first %d sectors.\r\n", sectors_to_clear);

    status = device->write((uint8_t *)&mbr_data, 1, 0, mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        FAT32_LOG_INFO("Error device write sector 0 (status: %d)!\r\n", status);
        return FAT32_ERR_WRITE_FAIL;
    }
    FAT32_LOG_INFO("Boot sector written successfully to sector 0.\r\n");

    status = device->write((uint8_t *)&mbr_data, 1, 6, mbr_data.BPB_BytsPerSec); //
    if (status < 0)
    {
        FAT32_LOG_INFO("Error device write sector 6 (status: %d)!\r\n", status);
        return FAT32_ERR_WRITE_FAIL;
    }
    FAT32_LOG_INFO("Boot sector backup written successfully to sector 6.\r\n");

    FSInfo_Type fs_info = {0};
    fs_info.FSI_LeadSig = LEAD_SIGNATURE;
    fs_info.FSI_StrucSig = STRUCT_SIGNATURE;
    fs_info.FSI_TrailSig = TRAIL_SIGNATURE;

    uint32_t free_count_claster = (mbr_data.BPB_TotSec32 - mbr_data.BPB_RsvdSecCnt - (mbr_data.BPB_FATSz32 * mbr_data.BPB_NumFATs)) / mbr_data.BPB_SecPerClus;
    fs_info.FSI_Free_Count = free_count_claster - 4;

    fs_info.FSI_Nxt_Free = 4; // 0 и 1 не должны использоваться
    status = device->write((uint8_t *)&fs_info, 1, 1, mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        FAT32_LOG_INFO("Error device write sector FSInfo (status: %d)!\r\n", status);
        return FAT32_ERR_WRITE_FAIL;
    }
    FAT32_LOG_INFO("FSInfo sector written successfully to sector 1.\r\n");

    // Заполение таблицы 1 и 2
    uint32_t tabl1_addr = mbr_data.BPB_RsvdSecCnt + mbr_data.BPB_HiddSec;
    uint32_t tabl2_addr = mbr_data.BPB_FATSz32 + tabl1_addr;

    uint8_t sectors_per_write = 10;
    int max_records = (mbr_data.BPB_BytsPerSec * sectors_per_write) / 4;

    uint32_t records[max_records];
    uint32_t idx = 0;
    for (idx = 0; idx < max_records; ++idx)
    {
        records[idx] = FREE_CLUSTER;
    }

    for (idx = 0; idx < mbr_data.BPB_FATSz32; idx += sectors_per_write)
    {
        status = device->write((uint8_t *)records, sectors_per_write, (tabl1_addr + idx), mbr_data.BPB_BytsPerSec);
        if (status < 0)
        {
            FAT32_LOG_INFO("Error device write sector of table 1 (sector: %d, status: %d)!\r\n",
                           tabl1_addr + idx, status);
            return FAT32_ERR_WRITE_FAIL;
        }
    }
    for (idx = 0; idx < mbr_data.BPB_FATSz32; idx += sectors_per_write)
    {
        status = device->write((uint8_t *)records, sectors_per_write, (tabl2_addr + idx), mbr_data.BPB_BytsPerSec);
        if (status < 0)
        {
            FAT32_LOG_INFO("Error device write sector of table 2 (sector: %d, status: %d)!\r\n",
                           tabl2_addr + idx, status);
            return FAT32_ERR_WRITE_FAIL;
        }
    }

    FAT32_LOG_INFO("Tables written successfully.\r\n");

    records[0] = RESERV_CLUSTER_FAT32;
    records[1] = FAT32_CLUSTER_END;
    records[2] = FAT32_CLUSTER_END;
    records[3] = FAT32_CLUSTER_END;

    status = device->write((uint8_t *)records, 1, tabl1_addr, mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        return FAT32_ERR_WRITE_FAIL;
    }
    status = device->write((uint8_t *)records, 1, tabl2_addr, mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        return FAT32_ERR_WRITE_FAIL;
    }

    FatDir_Type dir = {
        .DIR_Attr = ATTR_DIRECTORY,
        .DIR_FileSize = 0,
        .DIR_Name = {'M', 'Y', 'D', 'I', 'R', ' ', ' ', ' ', ' ', ' ', ' '},
        .DIR_FstClusHI = 0,
        .DIR_FstClusLO = 3,
    };

    uint32_t data_addr = tabl2_addr + mbr_data.BPB_FATSz32;

    status = device->write((uint8_t *)&dir, 1, (data_addr + (2 - mbr_data.BPB_RootClus) * mbr_data.BPB_SecPerClus), mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        return FAT32_ERR_WRITE_FAIL;
    }
    uint8_t buffer_data[512] = {0};

    FatDir_Type dir1 = {
        .DIR_Attr = ATTR_DIRECTORY,
        .DIR_FileSize = 0,
        .DIR_Name = {'.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        .DIR_FstClusHI = 0,
        .DIR_FstClusLO = 3,
    };
    FatDir_Type dir2 = {
        .DIR_Attr = ATTR_DIRECTORY,
        .DIR_FileSize = 0,
        .DIR_Name = {'.', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
        .DIR_FstClusHI = 0,
        .DIR_FstClusLO = 2,
    };
    stm_memcpy(buffer_data, (uint8_t *)&dir1, sizeof(dir1));
    stm_memcpy(buffer_data + sizeof(dir1), (uint8_t *)&dir2, sizeof(dir2));

    status = device->write(buffer_data, 1, (data_addr + (3 - mbr_data.BPB_RootClus) * mbr_data.BPB_SecPerClus), mbr_data.BPB_BytsPerSec);
    if (status < 0)
    {
        return FAT32_ERR_WRITE_FAIL;
    }
    return 0;
}

void init_fat_layout_info(MBR_Type *mbr_data)
{
    if (fat_info == NULL)
    {
        return;
    }
    stm_memcpy((uint8_t *)&fat_info->mbr_data, (uint8_t *)mbr_data, sizeof(MBR_Type));
    fat_info->bytesPerSec = mbr_data->BPB_BytsPerSec;
    fat_info->root_cluster = mbr_data->BPB_RootClus;
    fat_info->secPerClus = mbr_data->BPB_SecPerClus;
    fat_info->fat_ents_sec = mbr_data->BPB_BytsPerSec / 4;
    fat_info->sizeFAT = mbr_data->BPB_FATSz32;
    // Вычисляем адреса таблиц FAT
    fat_info->address_tabl1 = mbr_data->BPB_HiddSec + mbr_data->BPB_RsvdSecCnt;
    fat_info->address_tabl2 += fat_info->address_tabl1 + fat_info->sizeFAT;
    // Адрес начала области данных (регион данных)
    fat_info->address_region = fat_info->address_tabl2 + fat_info->sizeFAT;
}

int mount_fat32(BlockDevice *device)
{
    if (device == NULL)
    {
        return FAT32_ERR_INVALID_ARGUMENT;
    }

    fat_info = fat32_alloc(sizeof(FatLayoutInfo));
    if (fat_info == NULL)
    {
        // Вывести в лог
        return FAT32_ERR_ALLOC_FAILED;
    }
    fat_info->device = device;

    if (device->block_size < 512)
    {
        FAT32_LOG_INFO("Block size too small for FAT32!\r\n");
        return FAT32_ERR_UNSUPPORTED_BLOCK_SIZE;
    }

    uint8_t buffer[fat_info->device->block_size];
    int status = fat_info->device->read(buffer, 1, 0, device->block_size);
    if (status != 0)
    {
        return FAT32_ERR_READ_FAIL;
    }

    MBR_Type *mbr_data = (MBR_Type *)buffer;

    if (mbr_data->Signature_word != WORD_SIGNATURE)
    {
        FAT32_LOG_INFO("MBR not valid!\r\n");
        return FAT32_ERR_INVALID_MBR;
    }
    if (mbr_data->BPB_FATSz16 != 0)
    {
        FAT32_LOG_INFO("It isn't fat32!\r\n");
        return FAT32_ERR_NOT_FAT32;
    }
    // перерасчёт таблицы и памяти для провреки

    // Инициализация структуры
    init_fat_layout_info(mbr_data);

    status = fat_info->device->read(buffer, 1, 1, fat_info->bytesPerSec);
    if (status < 0)
    {
        return status;
    }
    return 0;
}

int clear_table_fat32()
{
    if (fat_info == NULL)
        return FAT32_ERR_FS_NOT_LOADED;

    uint32_t sector_size = fat_info->bytesPerSec;
    uint32_t *buffer = fat32_alloc(sector_size);

    if (buffer == NULL)
    {
        return FAT32_ERR_ALLOC_FAILED;
    }

    uint32_t fat_sectors = fat_info->sizeFAT;
    int status = 0;
    // Обработка первого сектора
    status = fat_info->device->read((uint8_t *)buffer, 1, fat_info->address_tabl1, sector_size);
    if (status < 0)
    {
        status = FAT32_ERR_READ_FAIL;
        goto cleanup;
    }

    if (sector_size > 16)
    {
        memset((uint8_t *)buffer + 4, 0x00, sector_size - 16);
    }
    // Запись в основную FAT таблицу
    status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl1, sector_size);
    if (status < 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

    // Запись в резервную FAT таблицу
    status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl2, sector_size);
    if (status < 0)
    {
        status = FAT32_ERR_WRITE_FAIL;
        goto cleanup;
    }

    // Обработка остальных секторов
    memset((uint8_t *)buffer, 0x00, sector_size);
    for (uint32_t sector = 1; sector < fat_sectors; ++sector)
    {
        status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl1 + sector, sector_size);
        if (status < 0)
        {
            status = FAT32_ERR_WRITE_FAIL;
            goto cleanup;
        }
        status = fat_info->device->write((uint8_t *)buffer, 1, fat_info->address_tabl2 + sector, sector_size);
        if (status < 0)
        {
            status = FAT32_ERR_WRITE_FAIL;
            goto cleanup;
        }
    }
cleanup:
    if (fat32_free(buffer, sector_size) != 0)
    {
        // Добавить в лог
    }

    return status;
}
