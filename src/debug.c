#include "fat32/debug.h"
#include "fat32/log_fat32.h"
#include <string.h>




void print_bytes_as_string(const char *label, const char *data, size_t size)
{
    if (!data) {
        FAT32_LOG_INFO("\t%s: <NULL>\r\n", label);
        return;
    }

    char buf[64] = {0};
    size_t len = (size < sizeof(buf)-1) ? size : sizeof(buf)-1;
    memcpy(buf, data, len);
    buf[len] = '\0';

    FAT32_LOG_INFO("\t%s: %s\r\n", label, buf);
}

void print_unicode(const char *label, const uint16_t *buffer, size_t length)
{
    if (!buffer) {
        FAT32_LOG_INFO("\t%s: <NULL>\r\n", label);
        return;
    }

    FAT32_LOG_INFO("\t%s: ", label);
    for (size_t i = 0; i < length; i++) {
        FAT32_LOG_INFO("%04X ", buffer[i]);
    }
    FAT32_LOG_INFO("\r\n");
}


int debug_print_mbr(BlockDevice *dev)
{
    if (!dev) {
        FAT32_LOG_INFO("Error: BlockDevice not initialized!\r\n");
        return -1;
    }

    uint8_t buffer[512] = {0};
    int read_bytes = dev->read(buffer, 1, 0, dev->block_size);
    if (read_bytes != 512) {
        FAT32_LOG_INFO("Error: Failed to read MBR sector!\r\n");
        return -1;
    }

    MBR_Type *mbr = (MBR_Type *)buffer;
    if (!mbr) {
        FAT32_LOG_INFO("Error: MBR pointer is NULL!\r\n");
        return -1;
    }

    FAT32_LOG_INFO("MBR:\r\n");
    FAT32_LOG_INFO("\tBPB_SecPerClus: %d, BPB_BytsPerSec: %d, BPB_RsvdSecCnt: %d\r\n",
                   mbr->BPB_SecPerClus, mbr->BPB_BytsPerSec, mbr->BPB_RsvdSecCnt);
    FAT32_LOG_INFO("\tBPB_NumFATs: %d, BPB_RootEntCnt: %d, BPB_HiddSec: %d\r\n",
                   mbr->BPB_NumFATs, mbr->BPB_RootEntCnt, mbr->BPB_HiddSec);
    FAT32_LOG_INFO("\tBPB_TotSec32: %d, BPB_FATSz32: %d, BPB_FSVer: %d\r\n",
                   mbr->BPB_TotSec32, mbr->BPB_FATSz32, mbr->BPB_FSVer);
    FAT32_LOG_INFO("\tBPB_RootClus: %d, BPB_FSInfo: %d, Signature_word: %d\r\n",
                   mbr->BPB_RootClus, mbr->BPB_FSInfo, mbr->Signature_word);
    FAT32_LOG_INFO("\tBPB_ExtFlags: %d, BPB_NumHeads: %d, BPB_SecPerTrk: %d, BPB_BkBootSec: %d\r\n",
                   mbr->BPB_ExtFlags, mbr->BPB_NumHeads, mbr->BPB_SecPerTrk, mbr->BPB_BkBootSec);
    FAT32_LOG_INFO("\tBS_jmpBoot: %02X %02X %02X\r\n",
                   mbr->BS_jmpBoot[0], mbr->BS_jmpBoot[1], mbr->BS_jmpBoot[2]);

    print_bytes_as_string("BS_OEMName",   (const char*)mbr->BS_OEMName, sizeof(mbr->BS_OEMName));
    print_bytes_as_string("BS_FilSysType",(const char*)mbr->BS_FilSysType, sizeof(mbr->BS_FilSysType));
    print_bytes_as_string("BS_VolLab",    (const char*)mbr->BS_VolLab, sizeof(mbr->BS_VolLab));

    return 0;
}


int debug_print_fsinfo(BlockDevice *dev, uint32_t fsinfo_sector, uint16_t bytes_per_sec)
{
    if (!dev) {
        FAT32_LOG_INFO("Error: BlockDevice not initialized!\r\n");
        return -1;
    }

    uint8_t buffer[512] = {0};
    int read_bytes = dev->read(buffer, 1, fsinfo_sector, bytes_per_sec);
    if (read_bytes != bytes_per_sec) {
        FAT32_LOG_INFO("Error: Failed to read FSInfo sector %u\r\n", fsinfo_sector);
        return -1;
    }

    FSInfo_Type *fsinfo = (FSInfo_Type *)buffer;
    if (!fsinfo) {
        FAT32_LOG_INFO("Error: FSInfo pointer is NULL!\r\n");
        return -1;
    }

    FAT32_LOG_INFO("FSInfo:\r\n");
    FAT32_LOG_INFO("\tFSI_Nxt_Free: %u, FSI_Free_Count: %u\r\n",
                   fsinfo->FSI_Nxt_Free, fsinfo->FSI_Free_Count);

    return 0;
}



int debug_dump_dir_sector(BlockDevice *dev, uint32_t sector, uint16_t bytes_per_sec)
{
    if (!dev) {
        FAT32_LOG_INFO("Error: BlockDevice not initialized!\r\n");
        return -1;
    }

    uint8_t buffer[512] = {0};
    int read_bytes = dev->read(buffer, 1, sector, bytes_per_sec);
    if (read_bytes != bytes_per_sec) {
        FAT32_LOG_INFO("Error: Failed to read sector %u\r\n", sector);
        return -1;
    }

    FAT32_LOG_INFO("Directory sector: %u\r\n", sector);

    for (uint16_t offset = 0; offset < bytes_per_sec; offset += sizeof(FatDir_Type)) {
        FatDir_Type *entry = (FatDir_Type *)&buffer[offset];

        if (entry->DIR_Name[0] == ENTRY_FREE_FULL_FAT32) {
            break;
        }
        else if (entry->DIR_Name[0] == ENTRY_FREE_FAT32) {
            FAT32_LOG_INFO("\tEntry is free!\r\n");
        }
        else {
            if ((entry->DIR_Attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
                 debug_print_lfn_entry((LDIR_Type *)entry);
            } else {
                debug_print_sfn_entry(entry);
            }
        }
    }

    return 0;
}


int debug_print_fat_sector(BlockDevice *dev, uint32_t sector, uint16_t bytes_per_sec)
{
    if (!dev) {
        FAT32_LOG_INFO("Error: BlockDevice not initialized!\r\n");
        return -1;
    }

    uint32_t buffer[128] = {0};
    int read_bytes = dev->read((uint8_t *)buffer, 1, sector, bytes_per_sec);
    if (read_bytes != bytes_per_sec) {
        FAT32_LOG_INFO("Error: Failed to read FAT sector %u\r\n", sector);
        return -1;
    }

    FAT32_LOG_INFO("FAT table sector: %u\r\n", sector);
    for (int i = 0; i < (bytes_per_sec / sizeof(uint32_t)); ++i) {
        FAT32_LOG_INFO("\tCluster %d: 0x%08X\r\n", i, buffer[i]);
    }

    return 0;
}


void debug_print_sfn_entry(const FatDir_Type *file)
{
    if (!file) {
        FAT32_LOG_INFO("DIR: <NULL>\r\n");
        return;
    }

    FAT32_LOG_INFO("Directory Entry:\r\n");
    print_bytes_as_string("DIR_Name", (const char*)file->DIR_Name, sizeof(file->DIR_Name));
    FAT32_LOG_INFO("\tDIR_Attr: %u, DIR_NTRes: %u, DIR_FstClusHI: %u, DIR_FstClusLO: %u, DIR_FileSize: %u\r\n",
                   file->DIR_Attr, file->DIR_NTRes, file->DIR_FstClusHI,
                   file->DIR_FstClusLO, file->DIR_FileSize);
}

void debug_print_lfn_entry(const LDIR_Type *entry)
{
    if (!entry) {
        FAT32_LOG_INFO("LDIR: <NULL>\r\n");
        return;
    }

    FAT32_LOG_INFO("Long Directory Entry (LFN):\r\n");
    FAT32_LOG_INFO("\tLDIR_Ord: %d, LDIR_Attr: %X, LDIR_Chksum: %d, LDIR_FstClusLO: %d, LDIR_Type: %d\r\n",
                   entry->LDIR_Ord, entry->LDIR_Attr, entry->LDIR_Chksum,
                   entry->LDIR_FstClusLO, entry->LDIR_Type);

    print_unicode("Name1", (const uint16_t*)entry->LDIR_Name1, sizeof(entry->LDIR_Name1)/2);
    print_unicode("Name2", (const uint16_t*)entry->LDIR_Name2, sizeof(entry->LDIR_Name2)/2);
    print_unicode("Name3", (const uint16_t*)entry->LDIR_Name3, sizeof(entry->LDIR_Name3)/2);
}