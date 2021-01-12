// send bytes to uart esp32 -> rockpis 

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <time.h>
#include <stdlib.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "responsive.h"
#include "esp_log.h"
#include "serial_config.h"

#define DEBUG 0

#define DEFAULT_VREF        1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES       256
#define STACK_SIZE          2048
#define UART_PORT_NUM       2


static const char* TAG = "MyESP";
static esp_adc_cal_characteristics_t *adc_chars;
//static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_MAX; //ADC_ATTEN_DB_0;
//static const adc_unit_t unit = ADC_UNIT_2;


typedef struct pot {
    uint8_t         id;
    adc_unit_t      unit;
    adc_channel_t   chan;
} potentiometer;


static void check_efuse(void) {
    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}


static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}


static void echo_task(void *arg)
{
    potentiometer *pot      = (potentiometer *)arg;
    uint8_t id              = (uint8_t)pot->id;
    adc_channel_t channel   = (adc_channel_t)pot->chan;
    const adc_unit_t unit   = (adc_unit_t)pot->unit;

    check_efuse();

    //adc1_config_width(width);
    //adc2_config_channel_atten((adc2_channel_t)channel, atten);

    if (unit == ADC_UNIT_1) {
        adc1_config_width(width);
        adc1_config_channel_atten((adc1_channel_t)channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    } 
    
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
    
    uint8_t data[] = { 0, 32, 0 };

    Responsive resp = {
        .analogResolution = 4096,
        .activityThreshold = 8.0,
        .edgeSnapEnable = true,
        .errorEMA = 0.0,
        .sleeping = false 
    };

    SerialBytes s_data;

    analog_responsive_begin(&resp, true, 0.01);

    while (1) {
        uint32_t adc_reading = 0;
        int raw;
        adc2_get_raw((adc2_channel_t)channel, width, &raw);

        if (unit == ADC_UNIT_1) {
            adc_reading = adc1_get_raw((adc1_channel_t)channel);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, width, &raw);
            adc_reading = raw;
        }

        analog_responsive_update(&resp, adc_reading);
        
        if (hasChanged(&resp)) {
            adc_reading = getValue(&resp);
            serial_unpack_bytes(&s_data, id, adc_reading);
            data[0] = s_data.leftmost;
            data[2] = s_data.rightmost;

#if DEBUG
            ESP_LOGI(TAG, "data: %d\t%d\n", s_data.leftmost, s_data.rightmost);
#else
            //uart_write_bytes(UART_PORT_NUM, (const char *) data, sizeof(data));
            serial_send_data((const char *) data);
#endif
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {

/*
    static potentiometer pot1 = {
        .id = 1,
        .chan = ADC2_CHANNEL_0
    };
*/

    // pins: 34, 35, 32, 33
    // channelss: 6,7,4,5
    // unit: 1

    adc_channel_t channels[4] = { ADC_CHANNEL_6, ADC_CHANNEL_7, ADC_CHANNEL_4, ADC_CHANNEL_5 };
    const adc_unit_t unit = ADC_UNIT_1;
    static potentiometer pot[4];

    serial_init(); 

    
    for (int i = 0; i < 4; i++) {
        pot[i].id = i;
        pot[i].unit = unit;
        pot[i].chan = channels[i];
        xTaskCreate(echo_task, "uart_echo_task", STACK_SIZE, (void *)&pot[i], i+2, NULL);
    }

    //xTaskCreate(echo_task, "uart_echo_task", STACK_SIZE, (void *)&pot1, 2, NULL);
}

//salvatore tecnico smart italia 
//3391774949

//negozio smart italia - Jessica 
//0116992920