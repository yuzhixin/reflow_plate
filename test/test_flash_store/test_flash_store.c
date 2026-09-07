#include <unity.h>
#include "flash_store.h"

static SystemConfig cfg;

void setUp(void) {
    flash_config_load_defaults(&cfg);
}

void tearDown(void) {}

void test_defaults_are_valid_record(void) {
    TEST_ASSERT_TRUE(flash_config_record_valid(&cfg));
    TEST_ASSERT_EQUAL_FLOAT(APP_DEFAULT_TARGET_TEMP_C, cfg.target_temp);
    TEST_ASSERT_EQUAL(SOLDER_HIGH_TEMP, cfg.solder_type);
}

void test_rejects_invalid_target(void) {
    cfg.target_temp = APP_TARGET_TEMP_MAX_C + 1.0f;
    cfg.checksum = flash_config_checksum(&cfg);
    TEST_ASSERT_FALSE(flash_config_settings_valid(&cfg));
    TEST_ASSERT_FALSE(flash_config_record_valid(&cfg));
}

void test_rejects_invalid_solder_type(void) {
    cfg.solder_type = (SolderType)99;
    cfg.checksum = flash_config_checksum(&cfg);
    TEST_ASSERT_FALSE(flash_config_settings_valid(&cfg));
}

void test_rejects_bad_checksum(void) {
    cfg.checksum ^= 0x55AA55AAUL;
    TEST_ASSERT_FALSE(flash_config_record_valid(&cfg));
}

void test_prepare_record_clamps_and_sets_sequence(void) {
    SystemConfig input = cfg;
    SystemConfig out;
    input.target_temp = 999.0f;
    flash_config_prepare_record(&out, &input, 42);
    TEST_ASSERT_EQUAL_FLOAT(APP_TARGET_TEMP_MAX_C, out.target_temp);
    TEST_ASSERT_EQUAL_UINT32(42, out.sequence);
    TEST_ASSERT_TRUE(flash_config_record_valid(&out));
}

void test_settings_equal_ignores_sequence_and_checksum(void) {
    SystemConfig other = cfg;
    other.sequence++;
    other.checksum ^= 1;
    TEST_ASSERT_TRUE(flash_config_settings_equal(&cfg, &other));
    other.target_temp += 5.0f;
    TEST_ASSERT_FALSE(flash_config_settings_equal(&cfg, &other));
}

void test_select_latest_normal_and_wrap(void) {
    SystemConfig a = cfg;
    SystemConfig b = cfg;
    flash_config_prepare_record(&a, &cfg, 10);
    flash_config_prepare_record(&b, &cfg, 11);
    TEST_ASSERT_EQUAL_PTR(&b, flash_config_select_latest(&a, &b));

    flash_config_prepare_record(&a, &cfg, 0xFFFFFFFFUL);
    flash_config_prepare_record(&b, &cfg, 0U);
    TEST_ASSERT_EQUAL_PTR(&b, flash_config_select_latest(&a, &b));
}

void test_save_skips_unchanged_config(void) {
    sys_config = cfg;
    TEST_ASSERT_EQUAL(FLASH_STORE_NO_CHANGE, flash_store_save(&cfg));
}

void test_save_rejects_invalid_config(void) {
    cfg.target_temp = APP_TARGET_TEMP_MAX_C + 100.0f;
    TEST_ASSERT_EQUAL(FLASH_STORE_INVALID_CONFIG, flash_store_save(&cfg));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_defaults_are_valid_record);
    RUN_TEST(test_rejects_invalid_target);
    RUN_TEST(test_rejects_invalid_solder_type);
    RUN_TEST(test_rejects_bad_checksum);
    RUN_TEST(test_prepare_record_clamps_and_sets_sequence);
    RUN_TEST(test_settings_equal_ignores_sequence_and_checksum);
    RUN_TEST(test_select_latest_normal_and_wrap);
    RUN_TEST(test_save_skips_unchanged_config);
    RUN_TEST(test_save_rejects_invalid_config);
    return UNITY_END();
}
