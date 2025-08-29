#pragma once

#include <stdint.h>

#pragma pack(push, 1)

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

#define FREE_CLUSTER 0x0
#define CLASTER_DAMAGE 0xFFFFFFF7
#define FAT32_CLUSTER_END 0x0FFFFFFF

#define SHORT_NAME_SIZE 11

// Date and Time FAT32

#define FAT32_SET_TIME(hours, min, sec) (((hours & 0x1f) << 11) | ((min & 0x3f) << 5) | ((sec & 0x1f) << 0))
#define FAT32_SET_DATE(day, month, year) (((year & 0x7f) << 9) | ((month & 0xF) << 5) | ((day & 0x1f) << 0))

#define FAT32_GET_HOUR(value) (((value) >> 11) & 0x1f)
#define FAT32_GET_MINUTE(value) (((value) >> 5) & 0x3f)
#define FAT32_GET_SEC(value) ((value) & 0x1f)

#define FAT32_GET_YEAR(value) (((value) >> 9) & 0x7f)
#define FAT32_GET_MONTH(value) (((value) >> 5) & 0x0f)
#define FAT32_GET_DAY(value) ((value) & 0x1f)

#pragma pack(push, 1)

typedef struct
{
    uint16_t year;
    uint16_t month;
    uint16_t day;
} FAT32_Date_Type;

typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} FAT32_Time_Type;

typedef struct
{
    FAT32_Date_Type date;
    FAT32_Time_Type time;
} Fat32_DateTime;

#pragma pack(pop)

typedef enum
{
    SIZE_1GB = 1073741824,
    SIZE_2GB = 2147483648,
    SIZE_4GB = 4294967296,
    SIZE_8GB = 8589934592,
    SIZE_16GB = 8589934592,
    SIZE_32GB = 34359738368
} StorageCapacity;

typedef enum
{
    /* ============================
       Path and name errors (-200..-209)
    ============================ */
    FAT32_ERR_INVALID_PATH = -200,   // Invalid path
    FAT32_ERR_FILE_NOT_FOUND = -201, // File not found
    FAT32_ERR_DIR_NOT_FOUND = -202,  // Directory not found
    FAT32_ERR_PATH_TOO_LONG = -203,  // Path name too long
    FAT32_ERR_NAME_TOO_LONG = -204,  // File/directory name too long
    FAT32_ERR_INVALID_CHAR = -205,   // Invalid character in name/path
    /* ============================
       Disk and cluster errors (-210..-219)
    ============================ */
    FAT32_ERR_DISK_FULL = -210,              // No space left on disk
    FAT32_ERR_CLUSTER_CHAIN_BROKEN = -211,   // Cluster chain broken
    FAT32_ERR_INVALID_CLUSTER = -212,        // Invalid cluster
    FAT32_ERR_INVALID_CLUSTER_CHAIN = -213,  // Invalid cluster chain
    FAT32_ERR_CLUSTER_ALLOC_FAIL = -214,     // Cluster allocation failed
    FAT32_ERR_UNSUPPORTED_BLOCK_SIZE = -215, // Unsupported block size

    /* ============================
       File / directory operation errors (-220..-229)
    ============================ */
    FAT32_ERR_INVALID_ARGUMENT = -220,  // Invalid argument
    FAT32_ERR_INVALID_FILE_MODE = -221, // Invalid file mode
    FAT32_ERR_NO_FREE_ENTRIES = -222,   // No free directory entries
    FAT32_ERR_OPEN_FAILED = -223,       // Failed to open
    FAT32_ERR_CREATE_FAILED = -224,     // Failed to create file/directory
    FAT32_ERR_DELETE_PROTECTED = -225,  // Object is protected from deletion
    FAT32_ERR_DIR_NOT_EMPTY = -226,     // Directory is not empty
    FAT32_ERR_IS_DIRECTORY = -227,      // Expected file but found directory
    FAT32_ERR_NOT_A_DIRECTORY = -228,   // Expected directory but found file
    FAT32_ERR_DELETE_FAIL = -229,       // Failed to delete

    /* ============================
       Filesystem integrity and recovery errors (-230..-239)
    ============================ */
    FAT32_ERR_INVALID_SEEK_MODE = -230,   // Invalid seek mode
    FAT32_ERR_INVALID_POSITION = -231,    // Invalid offset
    FAT32_ERR_UPDATE_FAILED = -232,       // Failed to update FAT tables
    FAT32_ERR_UPDATE_PARTIAL_FAIL = -233, // Only one FAT table updated
    FAT32_ERR_RECOVERY_FAILED = -234,     // Filesystem recovery failed
    FAT32_ERR_FS_NOT_LOADED = -235,       // Filesystem not initialized
    FAT32_ERR_INVALID_MBR = -236,         // Invalid MBR
    FAT32_ERR_NOT_FAT32 = -237,           // Not a FAT32 partition

    /* ============================
       Entry and metadata errors (-240..-249)
    ============================ */
    FAT32_ERR_ENTRY_NOT_FOUND = -240, // Entry not found
    FAT32_ERR_ENTRY_CORRUPTED = -241, // Corrupted entry
    FAT32_ERR_NOT_FOUND = -242,       // Generic "not found"

    /* ============================
       I/O errors (-250..-259)
    ============================ */
    FAT32_ERR_READ_FAIL = -250,    // Read failed
    FAT32_ERR_WRITE_FAIL = -251,   // Write failed
    FAT32_ERR_FLUSH_FAILED = -252, // Flush failed

    /* ============================
       Memory allocator errors (-300..-309)
    ============================ */
    FAT32_ERR_ALLOC_NO_MEMORY = -300,    // Out of memory
    FAT32_ERR_ALLOC_INVALID_ARG = -301,  // Invalid argument
    FAT32_ERR_ALLOC_OUT_OF_RANGE = -302, // Pointer out of managed range
    FAT32_ERR_ALLOC_FAILED = -303,       // Allocation failed
    FAT32_ERR_ALLOC_FREE_FAILED = -304   // Free failed

} Fat32Error;
