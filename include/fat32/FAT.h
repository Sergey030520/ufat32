#pragma once

#include <stdint.h>

#pragma pack(push, 1)

// Для карты памяти от 2гб до 8гб размер кластера 4кБайт

typedef struct
{
    uint8_t BS_jmpBoot[3];   // Инструкция для загрузчика
    uint8_t BS_OEMName[8];   // Название ОС или FS
    uint16_t BPB_BytsPerSec; // Размер сектора
    uint8_t BPB_SecPerClus;  // Кол-во секторов в кластере
    uint16_t BPB_RsvdSecCnt; // Кол-во зарезервированных секторов перед основными данными FS
    uint8_t BPB_NumFATs;     // Кол-во таблиц Fat
    uint16_t BPB_RootEntCnt; // Кол-во записей в корневом каталоге
    uint16_t BPB_TotSec16;   // Общее кол-во секторов в разделе для FAT16
    uint8_t BPB_Media;       // Тип носителя
    uint16_t BPB_FATSz16;    // Размер FAT в секторах для FAT16
    uint16_t BPB_SecPerTrk;  // Параметры геометрии диска
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;   // Кол-во скрытых секторов
    uint32_t BPB_TotSec32;  // Общее количество секторов в разделе
    uint32_t BPB_FATSz32;   // Размер FAT в секторах для FAT32
    uint16_t BPB_ExtFlags;  // Дополнительные флаги
    uint16_t BPB_FSVer;     // Версия файловой системы FAT32
    uint32_t BPB_RootClus;  // Номер первого кластера для корневой директории
    uint16_t BPB_FSInfo;    // Адрес на сектора с информацией о FS
    uint16_t BPB_BkBootSec; // Backup boot sector адрес сектора
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum; // Номер диска
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;       // Подпись загрузчика
    uint32_t BS_VolID;        // Идентификатор тома
    uint8_t BS_VolLab[11];    // Метка тома
    uint8_t BS_FilSysType[8]; // This string is informational only
    uint8_t Reserved[420];
    uint16_t Signature_word;
} MBR_Type;

#define WORD_SIGNATURE 0xAA55
#define TRAIL_SIGNATURE 0xAA550000 // в конце любого сектора
#define LEAD_SIGNATURE 0x41615252
#define STRUCT_SIGNATURE 0x61417272

#define RESERV_CLUSTER_FAT32 0xffffff8;
#define FILE_END_TABLE_FAT32 0xFFFFFFF
#define NOT_USED_CLUSTER_FAT32 0x0000000
#define HrdErrBitMask 0x04000000
#define ClnShutBitMask 0x08000000

typedef struct
{
    uint32_t FSI_LeadSig;
    uint32_t FSI_Reserved1[120];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count; //
    uint32_t FSI_Nxt_Free;
    uint32_t FSI_Reserved2[3]; //
    uint32_t FSI_TrailSig;
} FSInfo_Type;

typedef struct
{
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} FatDir_Type;

#define ENTRY_FREE_FAT32 0xE5
#define ENTRY_FREE_FULL_FAT32 0x00
#define LFN_ENTRY_LAST 0x40
#define LFN_SHORT_NAME 6
#define LFN_NAME_LENGTH 13

typedef struct
{
    uint8_t LDIR_Ord;
    uint8_t LDIR_Name1[10];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint8_t LDIR_Name2[12];
    uint16_t LDIR_FstClusLO;
    uint8_t LDIR_Name3[4];
} LDIR_Type;

typedef struct
{
    uint32_t cluster;
    uint32_t sector;
    uint16_t offset;
} DirEntryPosition;

#define MAX_NAME_SIZE 255

typedef struct
{
    uint32_t cluster_idx;
    uint32_t cluster_number;
    uint32_t sector_idx;
    uint16_t byte_offset;
} FilePos;

typedef struct
{
    DirEntryPosition entry_pos;
    uint32_t first_cluster;
    uint32_t size_bytes;
    FilePos position;
    uint8_t flags;
} FAT32_File;

typedef enum
{
    F_READ,
    F_WRITE,
    F_APPEND
} FILE_Mode;

typedef enum
{
    F_SEEK_SET = 0,
    F_SEEK_CUR,
    F_SEEK_END
} SEEK_Mode;

typedef enum
{
    DELETE_DIR_SAFE,
    DELETE_DIR_RECURSIVE
} DeleteDirMode;

#define MAX_SYMBOLS_ENTRY 13

#pragma pack(pop)

typedef enum
{
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME = 0x0F

} AttrDir;

#define ATTR_LONG_NAME_MASK (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY | ATTR_ARCHIVE)

typedef enum
{
    SIZE_1GB = 1073741824,
    SIZE_2GB = 2147483648,
    SIZE_4GB = 4294967296,
    SIZE_8GB = 8589934592,
    SIZE_16GB = 8589934592,
    SIZE_32GB = 34359738368
} StorageCapacity;

#define SET_TIME(hours, min, sec) (((hours & 0x1f) << 11) | ((min & 0x3f) << 5) | ((sec & 0x1f) << 0))
#define SET_DATE(day, month, year) (((year & 0x7f) << 9) | ((month & 0xF) << 5) | ((day & 0x1f) << 0))

#define GET_HOUR(value) ((value & 0x1f) >> 11)
#define GET_MINUTE(value) ((value & 0x3f) >> 5)
#define GET_SEC(value) (value & 0x1f)

#define GET_YEAR(value) ((value & 0x7f) >> 9)
#define GET_MONTH(value) ((value & 0xf) >> 5)
#define GET_DAY(value) (value & 0x1f)

#define FREE_CLUSTER 0x0
#define CLASTER_DAMAGE 0xFFFFFFF7
#define FAT32_CLUSTER_END 0x0FFFFFFF

#define SHORT_NAME_SIZE 11

typedef enum
{
    ERROR_INIT = -1,
    NO_SPACE_IN_CLUSTER = -2,
    FAT_CLUSTER_ALLOC_FAIL = -3
} ERROR_FAT32;

typedef struct
{

} RootDirectoryMap;

typedef int (*fs_read_t)(uint8_t *buffer, uint32_t size, uint32_t address);
typedef int (*fs_write_t)(const uint8_t *buffer, uint32_t size, uint32_t address);
typedef int (*fs_clear_t)(uint32_t offset, uint32_t size);

typedef struct {
    fs_read_t read;
    fs_write_t write;
    fs_clear_t clear;
} BlockDevice;


typedef struct
{
    MBR_Type mbr_data;
    uint32_t address_tabl1;
    uint32_t address_tabl2;
    uint32_t address_region;
    uint32_t root_cluster;
    uint32_t secPerClus;
    uint32_t fat_ents_sec;
    uint32_t bytesPerSec;
    uint32_t sizeFAT;
    BlockDevice *device;
} FatLayoutInfo;

typedef enum
{
    FAT_ERR_INVALID_PATH = -10, //
    FAT_ERR_FILE_NOT_FOUND = -11, // Файл не найден
    FAT_ERR_DIR_NOT_FOUND = -12,// Директория не найдена
    FAT_ERR_INVALID_ARGUMENT = -13,//Некорректный аргумент
    FAT_ERR_PATH_TOO_LONG = -14,//Длинное имя пути
    FAT_ERR_NAME_TOO_LONG = 15, //Длинное имя файла/директории
    FAT_ERR_DISK_FULL = -16, // 
    FAT_ERR_INVALID_SEEK_MODE = -17, //
    FAT_ERR_INVALID_POSITION = -18, // Позиция смещения выходит за пределы
    FAT_ERR_CLUSTER_CHAIN_BROKEN = -19, //  Цепочка кластеров файла нарушена 
    FAT_ERR_UPDATE_FAILED = -20, // Ошибка обновления fat таблиц,
    FAT_ERR_UPDATE_PARTIAL_FAIL = -21, // Обновлена только первая таблица
    FAT_ERR_RECOVERY_FAILED = -22, //Ошибка восстановления файловой системы 
    FAT_ERR_FS_NOT_LOADED = -23, //Файловая система не инициализирована
    FAT_ERR_ENTRY_NOT_FOUND = -24, //
    FAT_ERR_ENTRY_CORRUPTED = -25,
    FAT_ERR_READ_FAIL = -26,
    FAT_ERR_WRITE_FAIL = -27,
    FAT_ERR_FLUSH_FAILED = 28,
    FAT_ERR_INVALID_FILE_MODE = 29,
    FAT_ERR_NO_FREE_ENTRIES = -30,
    FAT_ERR_OPEN_FAILED = -31,
    FAT_ERR_CREATE_FAILED = -32,
    FAT_ERR_INVALID_CLUSTER = -33,
    FAT_ERR_INVALID_CLUSTER_CHAIN = -34,
    FAT_ERR_DELETE_PROTECTED = -35,
    FAT_ERR_DIR_NOT_EMPTY = -36,
    FAT_ERR_IS_DIRECTORY = -37,
    FAT_ERR_NOT_A_DIRECTORY = -38,
    FAT_ERR_DELETE_FAIL = -39,
    FAT_ERR_INVALID_MBR = -40,
    FAT_ERR_NOT_FAT32 = -41,
    FAT_ERR_NOT_FOUND = -42
} Fat32Error;

extern FatLayoutInfo *fat_info;



/**
 * Монтирует файловую систему FAT32.
 *
 * Инициализирует и подготавливает к работе FAT32.
 *
 * @return 0 при успешном монтировании, отрицательное значение при ошибке.
 */
int mount_fat32(BlockDevice *device);

/**
 * Форматирует накопитель под файловую систему FAT32.
 *
 * @param capacity Емкость накопителя в байтах.
 * @return 0 при успешном форматировании, отрицательное значение при ошибке.
 */
int formatted_fat32(BlockDevice *device, uint64_t capacity);

/**
 * Выводит содержимое директории по указанному пути.
 *
 * @param path Путь к директории.
 * @return 0 при успешном выполнении, иначе код ошибки.
 */
int list_directory_fat32(const char *path);

/** 
* @brief Сохраняет изменения в структуре файла на файловой системе FAT32.
 *
 * Обновляет метаданные файла, включая размер и время последней модификации,
 * и записывает обновлённую запись каталога на носитель.
 * Используется для синхронизации состояния файлового дескриптора с физическим уровнем.
 *
 * @param file Указатель на структуру FAT32_File, описывающую открытый файл.
 * @return 0 при успешном выполнении, иначе код ошибки.
 */

int flush_fat32(FAT32_File *file);

/**
 * Создаёт новую директорию в файловой системе FAT32.
 *
 * @param path Путь к создаваемой директории.
 * @return 0 при успешном создании, отрицательное значение при ошибке.
 */
int mkdir_fat32(char *path);

/**
 * Перемещает текущую позицию указателя чтения/записи в файле FAT32.
 *
 * Позволяет задать новую позицию относительно начала, конца или текущего положения.
 *
 * @param file Указатель на файл FAT32.
 * @param offset Смещение в байтах.
 * @param mode Режим смещения: SEEK_SET (от начала), SEEK_CUR (от текущей), SEEK_END (от конца).
 * @return 0 при успехе,
 *         FAT_ERR_INVALID_ARGUMENT если передан некорректный режим,
 *         FAT_ERR_INVALID_SEEK_MODE если передан неизвестный режим смещения, 
 *         FAT_ERR_INVALID_POSITION если позиция выходит за границы файла,
 *         FAT_ERR_CLUSTER_CHAIN_BROKEN если цепочка кластеров файла нарушена,
 *         другие коды ошибок при внутренних ошибках.
 */
int seek_file_fat32(FAT32_File *file, int32_t offset, SEEK_Mode mode);

/**
 * Открывает файл в файловой системе FAT32.
 *
 * Если файл не существует и установлен соответствующий режим, он может быть создан.
 *
 * @param path Путь к файлу.
 * @param file Указатель на указатель, куда будет сохранена структура открытого файла.
 * @param mode Режим открытия (например, чтение, запись, создание и т.д.).
 * @return 0 при успешном открытии, отрицательное значение при ошибке.
 */
int open_file_fat32(char *path, FAT32_File **file, uint8_t mode);

/**
 * Закрывает ранее открытый файл FAT32.
 *
 * Все несохранённые данные будут записаны в файловую систему.
 *
 * @param file Указатель на структуру файла, подлежащего закрытию.
 * @return 0 при успешном закрытии, отрицательное значение при ошибке.
 */
int close_file_fat32(FAT32_File **file);

/**
 * Возвращает текущую позицию указателя чтения/записи в файле FAT32.
 *
 * Используется для определения смещения от начала файла, на котором находится курсор.
 *
 * @param file Указатель на открытую структуру файла FAT32.
 * @return Текущее смещение в байтах от начала файла.
 */
uint32_t tell_fat32(FAT32_File *file);

/**
 * Ищет директорию по указанному пути в файловой системе FAT32.
 *
 * Возвращает кластер, соответствующий найденной директории, если путь корректен.
 *
 * @param path Путь к директории (например, "/folder/subfolder").
 * @param out_cluster Указатель на переменную, в которую будет записан номер кластера директории.
 * @return 0 при успешном поиске, отрицательное значение при ошибке (например, директория не найдена).
 */
int find_directory_fat32(char *path, uint32_t *out_cluster);

int get_dir_path(char *file_path, char *dir_path, int size);

/**
 * Удаляет файл по указанному пути из файловой системы FAT32.
 *
 * Файл удаляется из записи каталога, а его кластеры помечаются как свободные.
 *
 * @param path Полный путь к файлу (например, "/folder/file.txt").
 * @return 0 при успешном удалении, отрицательное значение при ошибке (например, файл не найден).
 */
int delete_file_fat32(char *path);

/**
 * Удаляет папку по указанному пути из файловой системы FAT32.
 *
 * Папка удаляется из записи каталога, а его кластеры помечаются как свободные.
 *
 * @param path Полный путь к файлу (например, "/folder/remove_folder").
 * @return 0 при успешном удалении, отрицательное значение при ошибке (например, файл не найден).
 */
int delete_dir_fat32(char *path, DeleteDirMode mode);

/**
 * Проверяет существование указанного пути в файловой системе FAT32.
 *
 * Может использоваться для проверки наличия файла или директории.
 *
 * @param path Полный путь (например, "/folder/file.txt" или "/folder").
 * @return 0 если путь существует, 1 если не существует.
 */
int path_exists_fat32(char *path);

/**
 * Читает данные из файла FAT32.
 *
 * Считывает указанное количество байт из текущей позиции файла в буфер.
 * После чтения позиция смещается на количество прочитанных байт.
 *
 * @param file Указатель на открытый файл FAT32.
 * @param buffer Буфер для хранения считанных данных.
 * @param size Количество байт для чтения.
 * @return Количество фактически прочитанных байт или отрицательное значение при ошибке.
 */
int read_file_fat32(FAT32_File *file, uint8_t *buffer, const uint32_t size);

/**
 * Записывает данные в файл FAT32.
 *
 * @param file    Указатель на структуру FAT32_File, описывающую открытый файл.
 * @param buffer  Буфер с данными, которые нужно записать.
 * @param length  Количество байт для записи.
 *
 * @return Количество успешно записанных байт, либо код ошибки (< 0).
 */
int write_file_fat32(FAT32_File *file, uint8_t *buffer, uint32_t length);

/**
 * @brief Очищает таблицы FAT (File Allocation Table) в файловой системе FAT32.
 *
 * Эта функция обнуляет все записи в основной и резервной FAT-таблицах, кроме первых 4-х байт (резервных),
 * тем самым удаляя всю информацию о распределении кластеров. Используется при форматировании или полной очистке данных.
 *
 * @return 0 при успехе, или код ошибки:
 *         - FAT_ERR_FS_NOT_LOADED — если файловая система не загружена
 *         - POOL_ERR_ALLOCATION_FAILED — ошибка выделения буфера
 *         - FAT_ERR_READ_FAIL — ошибка чтения с SD-карты
 */
int clear_table_fat32();

// test

int show_entry_fat32(uint32_t sector);
void clear_fat32();
int show_table_fat32(uint32_t sector);