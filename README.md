# rockpis_esp32_controller

software to manage esp32 analog input and serial output to rockpis

### Configurazione ESP32

```c
// La variabile UART_NUM_2 rappresenta la UART 2
#define TXD_PIN (GPIO_NUM_17) // pin di trasmissione
#define RXD_PIN (GPIO_NUM_16) // pin di ricezione
```

### Configurazione ROCKPIS

Nel file `/boot/uEnv.txt` il termine **rk3308-uart1** attiva la uart1 sui pin 23 e 24:

```bash
overlays=rk3308-console-on-uart0 rk3308-uart1 rk3308-i2c3
```

pin:

```
23 PIN DI RICEZIONE (UART1_RX)
24 PIN DI TRASMISSIONE (UART1_tX)
```

### Collegamenti

| ESP32        | Rockpis       |
| ------------ | ------------- |
| 17 (TXD_PIN) | 23 (UART1_RX) |
| 16 (RXD_PIN) | 24 (UART1_tX) |
| BAT          | 2 (+5V)       |
| GND          | 6 (GND)       |


## TODO
* verificare che l'ESP32 possa prendere i 5V direttamente dall'adattatore micro-USB. Il quel caso creare 2 bus: +5V e GND per alimentare sia la ROCKPIS che la ESP32