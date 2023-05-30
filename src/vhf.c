#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pal.pio.h"
#include "pal_data.pio.h"
#include "vhf.h"

uint32_t *transmit_buf;

void __isr dma_handler() {
    dma_hw->ints0 = DMA_CHANNEL_MASK;

    // pio_sm_set_enabled(PIO_INSTANCE, PAL_DATA_SM, false);
    // pio_sm_set_enabled(PIO_INSTANCE, PAL_SM, false);
    // pio_sm_clear_fifos(PIO_INSTANCE, PAL_DATA_SM);
    // pio_sm_clear_fifos(PIO_INSTANCE, PAL_SM);
    // pio_sm_restart(PIO_INSTANCE, PAL_DATA_SM);
    // pio_sm_restart(PIO_INSTANCE, PAL_SM);
    // pio_sm_set_enabled(PIO_INSTANCE, PAL_SM, true);
    // pio_sm_set_enabled(PIO_INSTANCE, PAL_DATA_SM, true);
    
    dma_channel_set_read_addr(DMA_CHANNEL, transmit_buf, true);
}

void dma_init() {
    dma_channel_config c = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(PIO_INSTANCE, PAL_DATA_SM, true));

    dma_channel_configure(DMA_CHANNEL, &c, &PIO_INSTANCE->txf[PAL_DATA_SM], transmit_buf, TRANSMIT_COUNT, true);
}


int main() {
    stdio_init_all();
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);

    transmit_buf = (uint32_t*) malloc(TRANSMIT_COUNT * sizeof(uint32_t));
    if (transmit_buf == NULL) {
        return 1;
    }

    for(int i = 0; i < TRANSMIT_COUNT / 10; i++) {
        if (i & 1 == 1) {
            for (int j = 0; j < 10; j++) {
                transmit_buf[((i * 10) + j)] = 0xFF00FF00;
            }
        } else {
            for (int j = 0; j < 10; j++) {
                transmit_buf[((i * 10) + j)] = 0x00FF00FF;
            }
        }

    }

    uint pal_data_offset = pio_add_program(PIO_INSTANCE, &pal_data_program);
    pal_data_program_init(PIO_INSTANCE, PAL_DATA_SM, pal_data_offset, DATA_PIN);
    uint pal_offset = pio_add_program(PIO_INSTANCE, &pal_program);
    pal_program_init(PIO_INSTANCE, PAL_SM, pal_offset, SYNC_PIN);

    pio_sm_set_enabled(PIO_INSTANCE, PAL_DATA_SM, true);
    pio_sm_set_enabled(PIO_INSTANCE, PAL_SM, true);
    dma_init(); 

    while (true) {
        tight_loop_contents();
    }
    free(transmit_buf);
    return 0;
}