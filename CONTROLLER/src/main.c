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
#include "serial_config.h"

#define DEBUG 0

#define DEFAULT_VREF        1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES       256
#define STACK_SIZE          2048
#define UART_PORT_NUM       0


static esp_adc_cal_characteristics_t *adc_chars;
//static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_MAX; //ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_2;


typedef struct pot {
    uint8_t         id;
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
    potentiometer *pot = (potentiometer *)arg;
    uint8_t id = (uint8_t)pot->id;
    adc_channel_t channel = (adc_channel_t)pot->chan;

    check_efuse();

    //adc1_config_width(width);
    adc2_config_channel_atten((adc2_channel_t)channel, atten);

    
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
    
    uint8_t data[] = { 0, 0 };

    static Responsive resp = {
        .analogResolution = 4096,
        .activityThreshold = 8.0,
        .edgeSnapEnable = true,
        .errorEMA = 0.0,
        .sleeping = false 
    };

    static SerialBytes s_data;

    analog_responsive_begin(&resp, true, 0.01);

    while (1) {
        uint32_t adc_reading = 0;
        int raw;
        adc2_get_raw((adc2_channel_t)channel, width, &raw);
        analog_responsive_update(&resp, raw);
        adc_reading = getValue(&resp);
        
        serial_unpack_bytes(&s_data, id, adc_reading);
        data[0] = s_data.leftmost;
        data[1] = s_data.rightmost;

#if DEBUG
        printf("id: %d\tvalue: %d\n", data[0], adc_reading);
        printf("unpacked values: %d, %d\n", 
            s_data.leftmost,
            s_data.rightmost
        );
#else
        uart_write_bytes(UART_PORT_NUM, (const char *) data, sizeof(data));
#endif
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {

    static potentiometer pot1 = {
        .id = 1,
        .chan = ADC_CHANNEL_3
    };

    run_serial_config(UART_PORT_NUM);
    
    xTaskCreate(echo_task, "uart_echo_task", STACK_SIZE, (void *)&pot1, 2, NULL);
}