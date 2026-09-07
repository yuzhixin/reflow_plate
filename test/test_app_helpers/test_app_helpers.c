#include <unity.h>
#include "app_config.h"

void setUp(void) {}
void tearDown(void) {}

void test_menu_wrap_prev_from_zero(void) {
    TEST_ASSERT_EQUAL_UINT8(2U, app_menu_prev(0U, APP_MAIN_MENU_ITEMS));
}

void test_menu_wrap_next_from_last(void) {
    TEST_ASSERT_EQUAL_UINT8(0U, app_menu_next((uint8_t)(APP_MAIN_MENU_ITEMS - 1U), APP_MAIN_MENU_ITEMS));
}

void test_menu_wrap_full_cycle_returns_to_start(void) {
    uint8_t index = 1U;
    index = app_menu_next(index, APP_MAIN_MENU_ITEMS);
    index = app_menu_next(index, APP_MAIN_MENU_ITEMS);
    index = app_menu_next(index, APP_MAIN_MENU_ITEMS);
    TEST_ASSERT_EQUAL_UINT8(1U, index);
}

void test_menu_wrap_zero_count_returns_zero(void) {
    TEST_ASSERT_EQUAL_UINT8(0U, app_menu_prev(3U, 0U));
    TEST_ASSERT_EQUAL_UINT8(0U, app_menu_next(3U, 0U));
}

void test_settings_menu_wrap_prev_from_zero(void) {
    TEST_ASSERT_EQUAL_UINT8((uint8_t)(APP_SETTINGS_ITEMS - 1U), app_menu_prev(0U, APP_SETTINGS_ITEMS));
}

void test_settings_menu_wrap_next_from_last(void) {
    TEST_ASSERT_EQUAL_UINT8(0U, app_menu_next((uint8_t)(APP_SETTINGS_ITEMS - 1U), APP_SETTINGS_ITEMS));
}

void test_app_clamp_float(void) {
    TEST_ASSERT_EQUAL_FLOAT(10.0f, app_clamp_float(5.0f, 10.0f, 20.0f));
    TEST_ASSERT_EQUAL_FLOAT(15.0f, app_clamp_float(15.0f, 10.0f, 20.0f));
    TEST_ASSERT_EQUAL_FLOAT(20.0f, app_clamp_float(25.0f, 10.0f, 20.0f));
}

void test_app_step_towards(void) {
    TEST_ASSERT_EQUAL_FLOAT(15.0f, app_step_towards(10.0f, 20.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(20.0f, app_step_towards(18.0f, 20.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(15.0f, app_step_towards(20.0f, 10.0f, 5.0f));
    TEST_ASSERT_EQUAL_FLOAT(10.0f, app_step_towards(12.0f, 10.0f, 5.0f));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_menu_wrap_prev_from_zero);
    RUN_TEST(test_menu_wrap_next_from_last);
    RUN_TEST(test_menu_wrap_full_cycle_returns_to_start);
    RUN_TEST(test_menu_wrap_zero_count_returns_zero);
    RUN_TEST(test_settings_menu_wrap_prev_from_zero);
    RUN_TEST(test_settings_menu_wrap_next_from_last);
    RUN_TEST(test_app_clamp_float);
    RUN_TEST(test_app_step_towards);
    return UNITY_END();
}
