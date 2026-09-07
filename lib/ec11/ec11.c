#include "ec11.h"
#include "app_config.h"

#ifndef PIO_UNIT_TESTING
#include "hw_config.h"
#include "systick.h"
#endif

static EC11_Event event_queue[APP_EC11_QUEUE_SIZE];
static uint8_t event_head = 0;
static uint8_t event_tail = 0;

// Encoder state (quadrature accumulator)
static uint8_t last_ab = 0;
static int8_t enc_counter = 0;

// Button state
static bool button_pressed = false;
static bool long_press_sent = false;
static uint32_t button_press_time = 0;

static void push_event(EC11_Event ev) {
    if (ev == EC11_EV_NONE) return;
    uint8_t next_head = (uint8_t)((event_head + 1U) % APP_EC11_QUEUE_SIZE);
    if (next_head == event_tail) {
        // Drop oldest event to keep the newest user action.
        event_tail = (uint8_t)((event_tail + 1U) % APP_EC11_QUEUE_SIZE);
    }
    event_queue[event_head] = ev;
    event_head = next_head;
}

void ec11_init(void) {
#ifndef PIO_UNIT_TESTING
    rcu_periph_clock_enable(RCU_GPIOA);
    // Initialize EC11 pins as input with pull-up
    gpio_mode_set(EC11_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, EC11_A_PIN | EC11_B_PIN | EC11_SW_PIN);

    uint8_t a = gpio_input_bit_get(EC11_PORT, EC11_A_PIN);
    uint8_t b = gpio_input_bit_get(EC11_PORT, EC11_B_PIN);
    last_ab = (a << 1) | b;
#endif
}

void ec11_scan_sample(uint8_t curr_a, uint8_t curr_b, uint8_t curr_sw, uint32_t now) {
    // 1. Full quadrature encoder with transition table
    uint8_t curr_ab = (uint8_t)((curr_a << 1) | curr_b);
    if (curr_ab != last_ab) {
        static const int8_t dir_table[4][4] = {
            // to: 00  01  10  11
            { 0,  1, -1,  0},  // from 00
            {-1,  0,  0,  1},  // from 01
            { 1,  0,  0, -1},  // from 10
            { 0, -1,  1,  0},  // from 11
        };
        enc_counter += dir_table[last_ab][curr_ab];
        if (enc_counter >= APP_EC11_STEPS_PER_DETENT) {
            push_event(EC11_EV_RIGHT);
            enc_counter -= APP_EC11_STEPS_PER_DETENT;
        } else if (enc_counter <= -APP_EC11_STEPS_PER_DETENT) {
            push_event(EC11_EV_LEFT);
            enc_counter += APP_EC11_STEPS_PER_DETENT;
        }
        last_ab = curr_ab;
    }

    // 2. Handle Button (Active Low)
    if (curr_sw == 0) { // Pressed
        if (!button_pressed) {
            button_pressed = true;
            long_press_sent = false;
            button_press_time = now;
        } else if (!long_press_sent && now - button_press_time >= APP_EC11_LONG_PRESS_MS) {
            long_press_sent = true;
            push_event(EC11_EV_LONG_PRESS);
        }
    } else { // Released
        if (button_pressed) {
            uint32_t held_ms = now - button_press_time;
            button_pressed = false;
            if (!long_press_sent && held_ms >= APP_EC11_DEBOUNCE_MS && held_ms < APP_EC11_LONG_PRESS_MS) {
                push_event(EC11_EV_CLICK);
            }
            long_press_sent = false;
        }
    }
}

void ec11_scan(void) {
#ifndef PIO_UNIT_TESTING
    uint8_t curr_a = gpio_input_bit_get(EC11_PORT, EC11_A_PIN);
    uint8_t curr_b = gpio_input_bit_get(EC11_PORT, EC11_B_PIN);
    uint8_t curr_sw = gpio_input_bit_get(EC11_PORT, EC11_SW_PIN);
    ec11_scan_sample(curr_a, curr_b, curr_sw, millis());
#endif
}

EC11_Event ec11_get_event(void) {
    if (event_head == event_tail) {
        return EC11_EV_NONE;
    }
    EC11_Event ev = event_queue[event_tail];
    event_tail = (uint8_t)((event_tail + 1U) % APP_EC11_QUEUE_SIZE);
    return ev;
}

#ifdef PIO_UNIT_TESTING
void ec11_reset_for_test(uint8_t a, uint8_t b, uint8_t sw) {
    event_head = 0;
    event_tail = 0;
    last_ab = (uint8_t)((a << 1) | b);
    enc_counter = 0;
    button_pressed = (sw == 0);
    long_press_sent = false;
    button_press_time = 0;
}
#endif
