#pragma once
#include <stdint.h>

const int MOCK_SECTOR_SIZE = 512;
const int MOCK_TOTAL_SECTORS = 1024;

void mock_device_init();
void mock_device_deinit();


int read_device(uint8_t *buffer, uint32_t size, uint32_t address);
int write_device(const uint8_t *buffer, uint32_t size, uint32_t address);
int clear_device(uint32_t sector_start, uint32_t sector_stop);
