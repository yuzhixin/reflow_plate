#include <unity.h>
#include "thermal_control.h"
#include "app_config.h"

void setUp(void) {}
void tearDown(void) {}

void test_ramp_target_steps_toward_operator_target(void) {
    float out = thermal_control_ramp_target(20.0f, 100.0f, 2.0f);
    TEST_ASSERT_EQUAL_FLOAT(22.0f, out);
    out = thermal_control_ramp_target(22.0f, 21.0f, 2.0f);
    TEST_ASSERT_EQUAL_FLOAT(21.0f, out);
}

void test_predictive_coasting_forces_zero(void) {
    ThermalControlState state;
    bool coasting = false;
    thermal_control_reset(&state);
    float out = thermal_control_limit_duty(&state, 80.0f, 100.0f, 96.0f, 2.0f, 0.1f, &coasting);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out);
    TEST_ASSERT_TRUE(coasting);
}

void test_near_target_caps_duty(void) {
    ThermalControlState state;
    bool coasting = false;
    thermal_control_reset(&state);
    state.last_duty = 30.0f;
    float out = thermal_control_limit_duty(&state, 90.0f, 100.0f, 92.0f, 0.0f, 1.0f, &coasting);
    TEST_ASSERT_LESS_OR_EQUAL(APP_DUTY_CAP_NARROW, out);
    TEST_ASSERT_FALSE(coasting);
}

void test_slew_limits_duty_rise(void) {
    ThermalControlState state;
    thermal_control_reset(&state);
    float out = thermal_control_limit_duty(&state, 100.0f, 200.0f, 20.0f, 0.0f, 0.05f, 0);
    TEST_ASSERT_EQUAL_FLOAT(APP_DUTY_SLEW_UP_PER_S * 0.05f, out);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ramp_target_steps_toward_operator_target);
    RUN_TEST(test_predictive_coasting_forces_zero);
    RUN_TEST(test_near_target_caps_duty);
    RUN_TEST(test_slew_limits_duty_rise);
    return UNITY_END();
}
