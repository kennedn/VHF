#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pal.h"
#include "vhf.h"
#include "benis.h"

uint32_t  *even, *odd;
uint even_odd = true;

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
    if (even_odd) {
        dma_channel_set_read_addr(DMA_CHANNEL, even, true);
        free(odd);
        odd = (uint32_t*) malloc(ODD_EVEN_BUFFER_SIZE * sizeof(uint32_t));
        if (odd == NULL) {
            panic("Could not allocate memory of odd buffer");
        }
        for (int i = 1; i < BUFF_HEIGHT * 2; i+=2) {
            for (int j = 0; j < TRANSMIT_COUNT_WIDTH; j++) {
                odd[(i >> 1) * TRANSMIT_COUNT_WIDTH + j] = framebuffer[i * TRANSMIT_COUNT_WIDTH + j];
            }
        }
    } else {
        dma_channel_set_read_addr(DMA_CHANNEL, odd, true);
        free(even);
        even = (uint32_t*) malloc(ODD_EVEN_BUFFER_SIZE * sizeof(uint32_t));
        if (even == NULL) {
            panic("Could not allocate memory of odd buffer");
        }
        for (int i = 0; i < BUFF_HEIGHT * 2; i+=2) {
            for (int j = 0; j < TRANSMIT_COUNT_WIDTH; j++) {
                even[(i >> 1) * TRANSMIT_COUNT_WIDTH + j] = framebuffer[i * TRANSMIT_COUNT_WIDTH + j];
            }
        }
    }
    even_odd = !even_odd;

}

void dma_init() {
    dma_channel_config c = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(PIO_INSTANCE, DATA_SM, true));

    even = (uint32_t*) malloc(ODD_EVEN_BUFFER_SIZE * sizeof(uint32_t));
    if (even == NULL) {
        panic("Could not allocate memory of even buffer");
    }
    odd = (uint32_t*) malloc(ODD_EVEN_BUFFER_SIZE * sizeof(uint32_t));
    if (odd == NULL) {
        panic("Could not allocate memory of odd buffer");
    }
    for (int i = 0; i < BUFF_HEIGHT * 2; i++) {
        for (int j = 0; j < TRANSMIT_COUNT_WIDTH; j++) {
            if (i & 1 == 1) {
                odd[(i >> 1) * TRANSMIT_COUNT_WIDTH + j] = framebuffer[i * TRANSMIT_COUNT_WIDTH + j];
            } else {
                even[(i >> 1) * TRANSMIT_COUNT_WIDTH + j] = framebuffer[i * TRANSMIT_COUNT_WIDTH + j];
            }
        }
    }
    dma_channel_configure(DMA_CHANNEL, &c, &PIO_INSTANCE->txf[DATA_SM], even, ODD_EVEN_BUFFER_SIZE, true);
}


int main() {
    stdio_init_all();
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);

    // framebuffer = (uint32_t*) malloc(FULL_BUFFER_SIZE * sizeof(uint32_t));
    // if (framebuffer == NULL) {
    //     return 1;
    // }
    // bool line_switch = false;
    // for(int i = 0; i < FULL_BUFFER_SIZE; i++) {
    //     if (i % 240 == 0) {
    //         line_switch = !line_switch;
    //     }
    //     if (line_switch) {
    //         framebuffer[i] = 0xFFFFFFFF;
    //     } else {
    //         framebuffer[i] = 0x00000000;
    //     }
    // }
    pal_init(PIO_INSTANCE, DATA_SM, SYNC_SM, DATA_PIN, SYNC_PIN, BUFF_WIDTH, BUFF_HEIGHT);
    dma_init(); 

    while (true) {
        // tight_loop_contents();
        // for(int i = 0; i < FULL_BUFFER_SIZE; i+=2) {
        //     framebuffer[i] = 0xFFFFFFFF;
        //     framebuffer[i+1] = 0x00000000;
        //     sleep_us(100);
        // }
        // for(int i = FULL_BUFFER_SIZE - 1; i > 0; i-=2) {
        //     framebuffer[i] = 0xFFFFFFFF;
        //     framebuffer[i+1] = 0x00000000;
        //     sleep_us(100);
        // }
        // for(int i = 0; i < FULL_BUFFER_SIZE; i+=2) {
        //     framebuffer[i] = 0x00000000;
        //     framebuffer[i+1] = 0xFFFFFFFF;
        //     sleep_us(100);
        // }
        // for(int i = FULL_BUFFER_SIZE - 1; i > 0; i-=2) {
        //     framebuffer[i] = 0x00000000;
        //     framebuffer[i+1] = 0xFFFFFFFF;
        //     sleep_us(100);
        // }
    }
    // free(framebuffer);
    return 0;
}