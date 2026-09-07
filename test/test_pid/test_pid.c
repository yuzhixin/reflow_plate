#include <unity.h>
#include "pid.h"

PID_Controller pid;

void setUp(void) {
    PID_Init(&pid, 2.0f, 0.5f, 1.0f, 0.0f, 100.0f);
}

void tearDown(void) {
}

void test_pid_initialization(void) {
    TEST_ASSERT_EQUAL_FLOAT(2.0f, pid.kp);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, pid.ki);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, pid.kd);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.out_min);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, pid.out_max);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.integral);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.prev_error);
    TEST_ASSERT_EQUAL(0, pid.has_prev_measurement);
}

void test_pid_reset_clears_state(void) {
    PID_Compute(&pid, 50.0f, 40.0f, 1.0f);
    PID_Reset(&pid);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.integral);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.prev_error);
    TEST_ASSERT_EQUAL(0, pid.has_prev_measurement);
}

void test_pid_compute_proportional_only(void) {
    PID_Init(&pid, 2.0f, 0.0f, 0.0f, 0.0f, 100.0f);
    float out = PID_Compute(&pid, 50.0f, 20.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(60.0f, out);
}

void test_pid_compute_clamping(void) {
    PID_Init(&pid, 5.0f, 0.0f, 0.0f, 0.0f, 100.0f);
    float out = PID_Compute(&pid, 50.0f, 20.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, out);
    out = PID_Compute(&pid, 20.0f, 50.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out);
}

void test_pid_compute_integral(void) {
    PID_Init(&pid, 0.0f, 1.0f, 0.0f, 0.0f, 100.0f);
    float out = PID_Compute(&pid, 50.0f, 40.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, out);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, pid.integral);
    out = PID_Compute(&pid, 50.0f, 40.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, out);
    TEST_ASSERT_EQUAL_FLOAT(20.0f, pid.integral);
}

void test_pid_derivative_on_measurement_avoids_target_step_kick(void) {
    PID_Init(&pid, 0.0f, 0.0f, 2.0f, -100.0f, 100.0f);
    float out = PID_Compute(&pid, 50.0f, 40.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out);
    out = PID_Compute(&pid, 80.0f, 40.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, out);
}

void test_pid_derivative_responds_to_temperature_change(void) {
    PID_Init(&pid, 0.0f, 0.0f, 2.0f, -100.0f, 100.0f);
    PID_Compute(&pid, 50.0f, 40.0f, 1.0f);
    float out = PID_Compute(&pid, 50.0f, 45.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(-10.0f, out);
}

void test_pid_conditional_anti_windup_blocks_saturation_growth(void) {
    PID_Init(&pid, 0.0f, 10.0f, 0.0f, 0.0f, 100.0f);
    float out = PID_Compute(&pid, 100.0f, 0.0f, 2.0f);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, out);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, pid.integral);
}

void test_pid_allows_integral_to_unwind_from_saturation(void) {
    PID_Init(&pid, 0.0f, 10.0f, 0.0f, 0.0f, 100.0f);
    pid.integral = 20.0f;
    float out = PID_Compute(&pid, 0.0f, 10.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, out);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, pid.integral);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_pid_initialization);
    RUN_TEST(test_pid_reset_clears_state);
    RUN_TEST(test_pid_compute_proportional_only);
    RUN_TEST(test_pid_compute_clamping);
    RUN_TEST(test_pid_compute_integral);
    RUN_TEST(test_pid_derivative_on_measurement_avoids_target_step_kick);
    RUN_TEST(test_pid_derivative_responds_to_temperature_change);
    RUN_TEST(test_pid_conditional_anti_windup_blocks_saturation_growth);
    RUN_TEST(test_pid_allows_integral_to_unwind_from_saturation);

    return UNITY_END();
}
