#include "interrupts.h"
#include "printf.h"
#include "timer.h"
#include "tivaware/hw_ints.h"
#include "tivaware/hw_ssi.h"
#include "tivaware/rom.h"
#include "tivaware/ssi.h"
#include "tivaware/sysctl.h"
#include "tivaware/udma.h"

uint8_t controltable[1024] __attribute__((aligned(1024)));

void udma_error_handler(void) {
    ROM_uDMAIntClear(INT_UDMAERR);
    printf("dma error\n\r");
}

uint8_t garbage = 0xFF;

void dma_recv(uint8_t* destbuf, uint32_t size) {
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC, (void*)(SSI0_BASE + SSI_O_DR),
                               destbuf, size);
    ROM_uDMAChannelTransferSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC, &garbage,
                               (void*)(SSI0_BASE + SSI_O_DR), size);

    ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0TX);
    ROM_uDMAChannelEnable(UDMA_CHANNEL_SSI0RX);
}

void initDMA(void) {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    while (!ROM_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA)) {}
    ROM_IntEnable(INT_UDMAERR);
    ROM_uDMAEnable();
    ROM_uDMAControlBaseSet(controltable);
    ROM_SSIDMAEnable(SSI0_BASE, SSI_DMA_RX | SSI_DMA_TX);
    ROM_SSIIntEnable(SSI0_BASE, SSI_DMARX | SSI_DMATX);
    ROM_IntEnable(INT_SSI0);

    ROM_uDMAChannelAttributeDisable(
        UDMA_CHANNEL_SSI0RX, UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                                 (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));
    ROM_uDMAChannelAttributeDisable(
        UDMA_CHANNEL_SSI0RX, UDMA_ATTR_USEBURST | UDMA_ATTR_ALTSELECT |
                                 (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));

    ROM_uDMAChannelControlSet(UDMA_CHANNEL_SSI0RX | UDMA_PRI_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                                  UDMA_ARB_4);
    ROM_uDMAChannelControlSet(UDMA_CHANNEL_SSI0TX | UDMA_PRI_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE |
                                  UDMA_DST_INC_NONE | UDMA_ARB_4);
}
