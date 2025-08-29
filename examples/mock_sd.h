#pragma once

#include "stdio.h"
#include "stdint.h"


int mock_sd_init();
int mock_sd_read(uint8_t *buffer, uint32_t size, uint32_t address, uint32_t timeout);
int mock_sd_write(const uint8_t *data, uint32_t size, uint32_t address, uint32_t timeout);
int mock_sd_erase(uint32_t address_start, uint32_t address_stop);