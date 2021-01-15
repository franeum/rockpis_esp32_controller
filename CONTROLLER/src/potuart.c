#include "driver/uart.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "responsive.h"
#include "esp_log.h"
#include "serial_config.h"
#include "potuart.h"


#define DEBUG               1
#define DEFAULT_VREF        1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define STACK_SIZE          2048
#define UART_PORT_NUM       2
#define NO_OF_SAMPLES       64


static const char* TAG = "MyESP";
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_bits_width_t WIDTH                 = ADC_WIDTH_BIT_12;
static const adc_atten_t ATTEN                      = ADC_ATTEN_DB_11; //ADC_ATTEN_DB_0;


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


void pot_to_uart_task(void *arg)
{
    potentiometer *pot      = (potentiometer *)arg;
    uint8_t id              = (uint8_t)pot->id;
    adc_channel_t channel   = (adc_channel_t)pot->chan;
    adc_unit_t unit         = (adc_unit_t)pot->unit;

    char str[10];

    check_efuse();

    if (unit == ADC_UNIT_1) {
        adc1_config_width(WIDTH);
        adc1_config_channel_atten(channel, ATTEN);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, ATTEN);
    } 
    
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, ATTEN, WIDTH, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);
    

    Responsive resp = {
        .analogResolution = 4096,
        .activityThreshold = 8.0,
        .edgeSnapEnable = true,
        .errorEMA = 0.0,
        .sleeping = false 
    };

    analog_responsive_begin(&resp, true, 0.01);
    
    while (1) {
        uint32_t adc_reading = 0;

        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            } else {
                int raw;
                adc2_get_raw((adc2_channel_t)channel, WIDTH, &raw);
                adc_reading += raw;
            }
        }

        uint32_t clean = adc_reading / NO_OF_SAMPLES;

        analog_responsive_update(&resp, clean);
        
        if (hasChanged(&resp)) {
            clean = getValue(&resp);
            sprintf(str, "%d %d\n", id, clean);

#if DEBUG
            ESP_LOGI(TAG, "%s", str);
#else
            serial_send_data((const char *)str);
#endif
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}