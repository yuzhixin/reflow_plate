#ifndef EC11_H
#define EC11_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    EC11_EV_NONE = 0,
    EC11_EV_LEFT,
    EC11_EV_RIGHT,
    EC11_EV_CLICK,
    EC11_EV_LONG_PRESS
} EC11_Event;

void ec11_init(void);

// Call this frequently (e.g. every 1ms or 5ms) to scan pins
void ec11_scan(void);

// Testable scan entry using active-low switch level and quadrature samples.
void ec11_scan_sample(uint8_t curr_a, uint8_t curr_b, uint8_t curr_sw, uint32_t now);

// Get the next queued event. Returns EC11_EV_NONE if nothing happened.
EC11_Event ec11_get_event(void);

#ifdef PIO_UNIT_TESTING
void ec11_reset_for_test(uint8_t a, uint8_t b, uint8_t sw);
#endif

#endif // EC11_H
