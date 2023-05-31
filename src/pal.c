#include "pal.pio.h"
#include "hardware/clocks.h"


static inline void sync_program_init(PIO pio, uint sm, uint offset, uint sync_pin, uint line_count) {
    pio_sm_config c = sync_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, sync_pin);
    pio_gpio_init(pio, sync_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, sync_pin, 1, true);
    pio_sm_set_pins_with_mask(pio, sm, (1u << sync_pin), (1u << sync_pin));
    
    // 1 ticks per 2us window 
    float div = clock_get_hz(clk_sys) / ((1 / 2.0e-6f));
    sm_config_set_clkdiv(&c, div);
    sm_config_set_out_shift(&c, false, false, 32);

    // Init the pio state machine with PC at offset
    pio_sm_init(pio, sm, offset, &c);

    // Put the number of video lines on the ISR
    pio_sm_put(pio, sm, line_count - 1);
    pio_sm_exec(pio, sm, 0x80a0);  // pull side 0
}

static inline void data_program_init(PIO pio, uint sm, uint offset, uint data_pin, uint pixel_count) {
    pio_sm_config c = data_program_get_default_config(offset);
    // sm_config_set_sideset_pins(&c, data_pin);
    sm_config_set_set_pins(&c, data_pin, 1);
    sm_config_set_out_pins(&c, data_pin, 1);
    pio_gpio_init(pio, data_pin);
    pio_sm_set_consecutive_pindirs(pio, sm, data_pin, 1, true);
    pio_sm_set_pins_with_mask(pio, sm, 0, (1u << data_pin));

    // frequency = clock ticks / time
    // divider = clock_frequency / frequency
    float div = clock_get_hz(clk_sys) / ((pixel_count * 2) / 52.0e-6f);
    sm_config_set_clkdiv(&c, div);

    sm_config_set_out_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    // Init the pio state machine with PC at offset
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_put(pio, sm, pixel_count - 1);
    pio_sm_exec(pio, sm, 0x80a0);  // pull;        Put value into OSR
    pio_sm_exec(pio, sm, 0xa0c7);  // mov isr osr; ISR is not used so we can use it as storage for pixel count
    pio_sm_exec(pio, sm, 0x6060);  // out null 32; discard OSR
}

void pal_init(PIO pio, uint data_sm, uint sync_sm, uint data_pin, uint sync_pin, uint pixel_count, uint line_count) {
    uint data_offset = pio_add_program(pio, &data_program);
    uint sync_offset = pio_add_program(pio, &sync_program);
    data_program_init(pio, data_sm, data_offset, data_pin, pixel_count);
    sync_program_init(pio, sync_sm, sync_offset, sync_pin, line_count);
    pio_set_sm_mask_enabled(pio, 0b0011, true);
}