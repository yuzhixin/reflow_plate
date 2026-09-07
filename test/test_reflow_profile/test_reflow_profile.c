#include <unity.h>
#include "reflow_profile.h"
#include "flash_store.h"
#include "app_config.h"

static uint32_t fake_ms;

uint32_t millis(void) {
    return fake_ms;
}

void setUp(void) {
    fake_ms = 0;
    flash_config_load_defaults(&sys_config);
    sys_config.solder_type = SOLDER_LOW_TEMP;
    reflow_stop();
}

void tearDown(void) {}

void test_reflow_starts_at_current_temp(void) {
    reflow_start(25.0f);
    ReflowTickResult r = reflow_tick(25.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_PREHEAT, r.state);
    TEST_ASSERT_EQUAL_FLOAT(25.0f, r.target_temp);
    TEST_ASSERT_TRUE(r.heater_allowed);
}

void test_preheat_target_ramps(void) {
    reflow_start(20.0f);
    fake_ms = 2000;
    ReflowTickResult r = reflow_tick(20.0f);
    TEST_ASSERT_EQUAL_FLOAT(23.0f, r.target_temp);
}

void test_transitions_to_soak_when_preheat_reached(void) {
    reflow_start(20.0f);
    fake_ms = 60000;
    ReflowTickResult r = reflow_tick(100.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_SOAK, r.state);
    TEST_ASSERT_TRUE(r.stage_changed);
}

void test_dwell_waits_for_actual_peak(void) {
    reflow_start(130.0f);
    fake_ms = 0;
    sys_config.solder_type = SOLDER_LOW_TEMP;
    // Force through stages using measured boundaries.
    ReflowTickResult r = reflow_tick(130.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_SOAK, r.state);
    r = reflow_tick(130.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_REFLOW, r.state);

    fake_ms += 40000;
    r = reflow_tick(150.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_REFLOW, r.state);

    r = reflow_tick(166.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_REFLOW, r.state);
    fake_ms += (uint32_t)(APP_REFLOW_DWELL_TIME_S * 1000.0f);
    r = reflow_tick(166.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_COOL, r.state);
}

void test_cool_disallows_heating_until_actual_cool(void) {
    reflow_start(130.0f);
    reflow_tick(130.0f);
    reflow_tick(130.0f);
    fake_ms = (uint32_t)(APP_REFLOW_PEAK_WAIT_TIMEOUT_S * 1000.0f + 1U);
    ReflowTickResult r = reflow_tick(140.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_COOL, r.state);
    TEST_ASSERT_FALSE(r.heater_allowed);

    fake_ms += 60000;
    r = reflow_tick(80.0f);
    TEST_ASSERT_EQUAL(REFLOW_STATE_COOL, r.state);
    TEST_ASSERT_FALSE(r.heater_allowed);

    r = reflow_tick(APP_REFLOW_COOL_END_C);
    TEST_ASSERT_EQUAL(REFLOW_STATE_DONE, r.state);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_reflow_starts_at_current_temp);
    RUN_TEST(test_preheat_target_ramps);
    RUN_TEST(test_transitions_to_soak_when_preheat_reached);
    RUN_TEST(test_dwell_waits_for_actual_peak);
    RUN_TEST(test_cool_disallows_heating_until_actual_cool);
    return UNITY_END();
}
