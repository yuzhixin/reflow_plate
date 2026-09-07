#include <unity.h>
#include "max6675.h"

void setUp(void) {}
void tearDown(void) {}

void test_decode_zero_c_is_valid(void) {
    float temp = -1.0f;
    TEST_ASSERT_TRUE(max6675_decode_raw(0x0000, &temp));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, temp);
}

void test_decode_fractional_step(void) {
    float temp = 0.0f;
    TEST_ASSERT_TRUE(max6675_decode_raw(0x0008, &temp));
    TEST_ASSERT_EQUAL_FLOAT(0.25f, temp);
}

void test_decode_100c(void) {
    float temp = 0.0f;
    TEST_ASSERT_TRUE(max6675_decode_raw((uint16_t)(400U << 3), &temp));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, temp);
}

void test_decode_open_thermocouple(void) {
    float temp = 0.0f;
    TEST_ASSERT_EQUAL(MAX6675_FAULT_OPEN, max6675_decode(0x0004, &temp));
}

void test_decode_all_ones_invalid(void) {
    float temp = 0.0f;
    TEST_ASSERT_EQUAL(MAX6675_FAULT_ALL_ONES, max6675_decode(0xFFFF, &temp));
}

void test_decode_reserved_bits_invalid(void) {
    float temp = 0.0f;
    TEST_ASSERT_EQUAL(MAX6675_FAULT_SHORT_FRAME, max6675_decode(0x8000, &temp));
}

void test_decode_ignores_low_status_noise_bits(void) {
    float temp = 0.0f;
    TEST_ASSERT_EQUAL(MAX6675_OK, max6675_decode(0x0001, &temp));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, temp);
}

void test_decode_valid_high_temperature_boundary(void) {
    float temp = 0.0f;
    TEST_ASSERT_EQUAL(MAX6675_OK, max6675_decode((uint16_t)(1023U << 3), &temp));
    TEST_ASSERT_EQUAL_FLOAT(255.75f, temp);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_decode_zero_c_is_valid);
    RUN_TEST(test_decode_fractional_step);
    RUN_TEST(test_decode_100c);
    RUN_TEST(test_decode_open_thermocouple);
    RUN_TEST(test_decode_all_ones_invalid);
    RUN_TEST(test_decode_reserved_bits_invalid);
    RUN_TEST(test_decode_ignores_low_status_noise_bits);
    RUN_TEST(test_decode_valid_high_temperature_boundary);
    return UNITY_END();
}
