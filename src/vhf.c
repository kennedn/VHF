#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "pal.h"
#include "vhf.h"
#include "totoro.h"

uint32_t *framebuffer;
uint32_t  *even, *odd;
uint even_odd = true;

uint32_t* generate_half_buffer(uint32_t *buffer, bool odd) {
    uint32_t* half_buf = (uint32_t*) malloc(HALF_BUFFER_SIZE * sizeof(uint32_t));
    if (half_buf == NULL) {
        panic("Could not allocate memory for buffer");
    }
    for (int i = odd; i < FULL_BUFF_HEIGHT; i+=2) {
        for (int j = 0; j < DATA_COUNT_PER_ROW; j++) {
            half_buf[(i >> 1) * DATA_COUNT_PER_ROW + j] = buffer[i * DATA_COUNT_PER_ROW + j];
        }
    }
    return half_buf;
}

void __isr dma_handler() {
    if (even_odd) {
        dma_channel_set_read_addr(DMA_CHANNEL, even, true);
        free(odd);
        odd = generate_half_buffer(framebuffer, true);
    } else {
        dma_channel_set_read_addr(DMA_CHANNEL, odd, true);
        free(even);
        even = generate_half_buffer(framebuffer, false);
    }

    even_odd = !even_odd;
    dma_hw->ints0 = DMA_CHANNEL_MASK;
}

void dma_init() {
    dma_channel_config c = dma_channel_get_default_config(DMA_CHANNEL);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(PIO_INSTANCE, DATA_SM, true));

    even = generate_half_buffer(framebuffer, false);
    odd = generate_half_buffer(framebuffer, true);
    dma_channel_configure(DMA_CHANNEL, &c, &PIO_INSTANCE->txf[DATA_SM], even, HALF_BUFFER_SIZE, false);
}


int main() {
    stdio_init_all();
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    dma_channel_set_irq0_enabled(DMA_CHANNEL, true);
    irq_set_enabled(DMA_IRQ_0, true);

    framebuffer = (uint32_t*) malloc(FULL_BUFFER_SIZE * sizeof(uint32_t));
    if (framebuffer == NULL) {
        return 1;
    }
    dma_init(); 
    pal_init(PIO_INSTANCE, DATA_SM, SYNC_SM, DATA_PIN, SYNC_PIN, BUFF_WIDTH, HALF_BUFF_HEIGHT);
    dma_handler();
    
    bool line_switch = false;
    while (true) {
        // tight_loop_contents();
        for(int i = 0; i < FULL_BUFFER_SIZE; i+=2) {
            framebuffer[i] = 0xFFFFFFFF;
            framebuffer[i+1] = 0x00000000;
            sleep_us(100);
        }
        for(int i = FULL_BUFFER_SIZE - 1; i > 0; i-=2) {
            framebuffer[i] = 0xFFFFFFFF;
            framebuffer[i+1] = 0x00000000;
            sleep_us(100);
        }
        for(int i = 0; i < FULL_BUFFER_SIZE; i++) {
            if (i % 640 == 0) {
                line_switch = !line_switch;
            }
            if (line_switch) {
                framebuffer[i] = 0xFFFFFFFF;
            } else {
                framebuffer[i] = 0x00000000;
            }
            sleep_us(50);

        }
        for(int i = FULL_BUFFER_SIZE - 1; i > 0; i--) {
            if (i % 640 == 0) {
                line_switch = !line_switch;
            }
            if (line_switch) {
                framebuffer[i] = 0x00000000;
            } else {
                framebuffer[i] = 0xFFFFFFFF;
            }
            sleep_us(50);
        }
        for(int i = 0; i < FULL_BUFF_HEIGHT; i++) {
            memcpy(&framebuffer[i * DATA_COUNT_PER_ROW], &totoro[i * DATA_COUNT_PER_ROW], DATA_COUNT_PER_ROW * 4);
            sleep_ms(2);
        }
        for(int i = FULL_BUFFER_SIZE - 1; i > 0; i--) {
            framebuffer[i] = 0xFFFFFFFF;
            sleep_us(50);
        }
        for(int i = FULL_BUFF_HEIGHT - 1; i > 0; i--) {
            memcpy(&framebuffer[i * DATA_COUNT_PER_ROW], &totoro[i * DATA_COUNT_PER_ROW], DATA_COUNT_PER_ROW * 4);
            sleep_ms(2);
        }
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