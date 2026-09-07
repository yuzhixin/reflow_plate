#include <unity.h>
#include "ec11.h"
#include "app_config.h"

void setUp(void) {
    ec11_reset_for_test(0, 0, 1);
}

void tearDown(void) {}

void test_click_emits_once_after_debounce(void) {
    ec11_scan_sample(0, 0, 0, 10);
    ec11_scan_sample(0, 0, 1, 80);
    TEST_ASSERT_EQUAL(EC11_EV_CLICK, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_NONE, ec11_get_event());
}

void test_long_press_emits_once_and_no_release_click(void) {
    ec11_scan_sample(0, 0, 0, 0);
    ec11_scan_sample(0, 0, 0, APP_EC11_LONG_PRESS_MS);
    ec11_scan_sample(0, 0, 0, APP_EC11_LONG_PRESS_MS + 100);
    ec11_scan_sample(0, 0, 1, APP_EC11_LONG_PRESS_MS + 200);

    TEST_ASSERT_EQUAL(EC11_EV_LONG_PRESS, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_NONE, ec11_get_event());
}

void test_rotation_right_and_left(void) {
    ec11_scan_sample(0, 1, 1, 1);
    ec11_scan_sample(1, 1, 1, 2);
    ec11_scan_sample(1, 0, 1, 3);
    ec11_scan_sample(0, 0, 1, 4);
    TEST_ASSERT_EQUAL(EC11_EV_RIGHT, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_NONE, ec11_get_event());

    ec11_scan_sample(1, 0, 1, 5);
    ec11_scan_sample(1, 1, 1, 6);
    ec11_scan_sample(0, 1, 1, 7);
    ec11_scan_sample(0, 0, 1, 8);
    TEST_ASSERT_EQUAL(EC11_EV_LEFT, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_NONE, ec11_get_event());
}

void test_queue_preserves_rotation_and_click(void) {
    ec11_scan_sample(0, 1, 1, 1);
    ec11_scan_sample(1, 1, 1, 2);
    ec11_scan_sample(1, 0, 1, 3);
    ec11_scan_sample(0, 0, 0, 4);
    ec11_scan_sample(0, 0, 1, 80);

    TEST_ASSERT_EQUAL(EC11_EV_RIGHT, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_CLICK, ec11_get_event());
    TEST_ASSERT_EQUAL(EC11_EV_NONE, ec11_get_event());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_click_emits_once_after_debounce);
    RUN_TEST(test_long_press_emits_once_and_no_release_click);
    RUN_TEST(test_rotation_right_and_left);
    RUN_TEST(test_queue_preserves_rotation_and_click);
    return UNITY_END();
}
