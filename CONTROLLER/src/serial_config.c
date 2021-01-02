#include "driver/uart.h"
#include "serial_config.h"


#define ECHO_TEST_TXD (4)
#define ECHO_TEST_RXD (5)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
#define ECHO_UART_BAUD_RATE     (115200)
#define BUF_SIZE (1024)


void run_serial_config(int number) {
    uart_config_t uart_config = {
        .baud_rate      = ECHO_UART_BAUD_RATE,
        .data_bits      = UART_DATA_8_BITS,
        .parity         = UART_PARITY_DISABLE,
        .stop_bits      = UART_STOP_BITS_1,
        .flow_ctrl      = UART_HW_FLOWCTRL_DISABLE,
        .source_clk     = UART_SCLK_APB,
    };

    int intr_alloc_flags = 0;
        

    #if CONFIG_UART_ISR_IN_IRAM
        intr_alloc_flags = ESP_INTR_FLAG_IRAM;
    #endif

    ESP_ERROR_CHECK(uart_driver_install(number, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(number, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(number, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}


void serial_unpack_bytes(SerialBytes * x, uint8_t id, uint32_t value) {
    x->rightmost = (uint8_t)(value & 255);
    uint8_t e_id = id << 4;
    x->leftmost = (uint8_t)(((value >> 8) & 255) | e_id);
}