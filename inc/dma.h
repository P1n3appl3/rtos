#pragma once

#include <stdint.h>

void initDMA(void);

void dma_recv(uint8_t* destbuf, uint32_t size);
