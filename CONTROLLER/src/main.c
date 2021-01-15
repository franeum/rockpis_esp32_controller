// send bytes to uart esp32 -> rockpis 

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc_cal.h"
#include "serial_config.h"
#include "potuart.h"


#define DEBUG               1
#define STACK_SIZE          2048


void app_main(void) {

    adc_channel_t channels[4]   = { ADC_CHANNEL_9, ADC_CHANNEL_8, ADC_CHANNEL_5, ADC_CHANNEL_4 };
    adc_unit_t unit[4]          = { ADC_UNIT_2, ADC_UNIT_2, ADC_UNIT_1, ADC_UNIT_1,  };
    static potentiometer pot[4];

    serial_init(); 

    
    for (int i = 0; i < 4; i++) {
        pot[i].id = i;
        pot[i].unit = unit[i];
        pot[i].chan = channels[i];
        xTaskCreate(pot_to_uart_task, "pot_uart_task", STACK_SIZE, (void *)&pot[i], 1000+i, NULL);      
    }
}



//salvatore tecnico smart italia 
//3391774949

//negozio smart italia - Jessica 
//0116992920