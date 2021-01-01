
typedef struct serial_bytes {
    uint8_t id;
    uint8_t leftmost;
    uint8_t rightmost; 
} SerialBytes;

void serial_unpack_bytes(SerialBytes * x, uint8_t id, uint32_t value);
void run_serial_config(int number);