#include "driver/adc.h"

typedef struct pot {
    uint8_t         id;
    adc_unit_t      unit;
    adc_channel_t   chan;
} potentiometer;

void pot_to_uart_task(void *arg);