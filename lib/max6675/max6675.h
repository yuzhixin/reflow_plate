#ifndef MAX6675_H
#define MAX6675_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MAX6675_OK = 0,
    MAX6675_FAULT_OPEN,
    MAX6675_FAULT_SHORT_FRAME,
    MAX6675_FAULT_ALL_ONES,
    MAX6675_FAULT_JUMP
} Max6675Status;

typedef struct {
    Max6675Status status;
    float temp_c;
    uint16_t raw;
} Max6675Reading;

// Initialize the GPIOs for MAX6675 (SCK, SO, CS)
void max6675_init(void);

// Decode a raw 16-bit MAX6675 frame. Returns false when the frame is a fault.
bool max6675_decode_raw(uint16_t raw, float *out_temp);
Max6675Status max6675_decode(uint16_t raw, float *out_temp);

// Read temperature with status.
Max6675Reading max6675_read(void);

// Compatibility wrapper. Returns -1.0 if thermocouple is disconnected/invalid.
float max6675_read_temp(void);

#endif // MAX6675_H
