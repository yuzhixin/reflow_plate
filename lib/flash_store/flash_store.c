#include "flash_store.h"
#include <stddef.h>
#include <string.h>

#ifndef PIO_UNIT_TESTING
#include "gd32e23x.h"
#endif

#define CONFIG_VERSION 1UL
#define FNV1A_OFFSET   2166136261UL
#define FNV1A_PRIME    16777619UL

SystemConfig sys_config;

#ifndef PIO_UNIT_TESTING
extern const uint8_t __config_page_a[];
extern const uint8_t __config_page_b[];
#define CONFIG_PAGE_A ((uint32_t)__config_page_a)
#define CONFIG_PAGE_B ((uint32_t)__config_page_b)
#else
#define CONFIG_PAGE_A 0x0800F800UL
#define CONFIG_PAGE_B 0x0800FC00UL
#endif

static uint32_t current_page_addr = CONFIG_PAGE_A;

static bool solder_type_valid(SolderType type) {
    return type >= SOLDER_LOW_TEMP && type <= SOLDER_HIGH_TEMP;
}

uint32_t flash_config_checksum(const SystemConfig *cfg) {
    SystemConfig tmp;
    memcpy(&tmp, cfg, sizeof(tmp));
    tmp.checksum = 0;

    const uint8_t *ptr = (const uint8_t *)&tmp;
    uint32_t hash = FNV1A_OFFSET;
    for (uint32_t i = 0; i < sizeof(tmp); i++) {
        hash ^= ptr[i];
        hash *= FNV1A_PRIME;
    }
    return hash;
}

bool flash_config_settings_valid(const SystemConfig *cfg) {
    if (cfg == NULL) return false;
    if (!app_target_temp_is_valid(cfg->target_temp)) return false;
    if (!app_pid_gain_is_valid(cfg->pid_kp)) return false;
    if (!app_pid_gain_is_valid(cfg->pid_ki)) return false;
    if (!app_pid_gain_is_valid(cfg->pid_kd)) return false;
    if (!solder_type_valid(cfg->solder_type)) return false;
    return true;
}

bool flash_config_record_valid(const SystemConfig *cfg) {
    if (cfg == NULL) return false;
    if (cfg->magic != APP_FLASH_CONFIG_MAGIC) return false;
    if (cfg->version != CONFIG_VERSION) return false;
    if (!flash_config_settings_valid(cfg)) return false;
    return flash_config_checksum(cfg) == cfg->checksum;
}

bool flash_config_settings_equal(const SystemConfig *a, const SystemConfig *b) {
    if (a == NULL || b == NULL) return false;
    return a->target_temp == b->target_temp &&
           a->pid_kp == b->pid_kp &&
           a->pid_ki == b->pid_ki &&
           a->pid_kd == b->pid_kd &&
           a->solder_type == b->solder_type;
}

void flash_config_prepare_record(SystemConfig *dst, const SystemConfig *settings, uint32_t sequence) {
    memset(dst, 0, sizeof(*dst));
    dst->magic = APP_FLASH_CONFIG_MAGIC;
    dst->version = CONFIG_VERSION;
    dst->target_temp = app_clamp_target_temp(settings->target_temp);
    dst->pid_kp = settings->pid_kp;
    dst->pid_ki = settings->pid_ki;
    dst->pid_kd = settings->pid_kd;
    dst->solder_type = solder_type_valid(settings->solder_type) ? settings->solder_type : SOLDER_HIGH_TEMP;
    dst->sequence = sequence;
    dst->checksum = flash_config_checksum(dst);
}

void flash_config_load_defaults(SystemConfig *cfg) {
    SystemConfig defaults;
    memset(&defaults, 0, sizeof(defaults));
    defaults.target_temp = APP_DEFAULT_TARGET_TEMP_C;
    defaults.pid_kp = APP_PID_DEFAULT_KP;
    defaults.pid_ki = APP_PID_DEFAULT_KI;
    defaults.pid_kd = APP_PID_DEFAULT_KD;
    defaults.solder_type = SOLDER_HIGH_TEMP;
    flash_config_prepare_record(cfg, &defaults, 0);
}

static bool sequence_newer(uint32_t a, uint32_t b) {
    return (int32_t)(a - b) > 0;
}

const SystemConfig *flash_config_select_latest(const SystemConfig *a, const SystemConfig *b) {
    bool a_valid = flash_config_record_valid(a);
    bool b_valid = flash_config_record_valid(b);

    if (a_valid && b_valid) {
        return sequence_newer(a->sequence, b->sequence) ? a : b;
    }
    if (a_valid) return a;
    if (b_valid) return b;
    return NULL;
}

void flash_store_init(void) {
    const SystemConfig *cfg_a = (const SystemConfig *)CONFIG_PAGE_A;
    const SystemConfig *cfg_b = (const SystemConfig *)CONFIG_PAGE_B;
    const SystemConfig *latest = flash_config_select_latest(cfg_a, cfg_b);

    if (latest == cfg_a) {
        memcpy(&sys_config, cfg_a, sizeof(SystemConfig));
        current_page_addr = CONFIG_PAGE_A;
    } else if (latest == cfg_b) {
        memcpy(&sys_config, cfg_b, sizeof(SystemConfig));
        current_page_addr = CONFIG_PAGE_B;
    } else {
        flash_config_load_defaults(&sys_config);
        current_page_addr = CONFIG_PAGE_B; // Will write to A on next save
    }
}

#ifndef PIO_UNIT_TESTING
static void flash_clear_flags(void) {
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR | FMC_FLAG_PGAERR);
}
#endif

FlashStoreResult flash_store_save(const SystemConfig *cfg) {
    if (!flash_config_settings_valid(cfg)) {
        return FLASH_STORE_INVALID_CONFIG;
    }

    if (flash_config_settings_equal(cfg, &sys_config)) {
        return FLASH_STORE_NO_CHANGE;
    }

#ifdef PIO_UNIT_TESTING
    flash_config_prepare_record(&sys_config, cfg, sys_config.sequence + 1U);
    return FLASH_STORE_OK;
#else
    uint32_t target_page = (current_page_addr == CONFIG_PAGE_A) ? CONFIG_PAGE_B : CONFIG_PAGE_A;
    SystemConfig record;
    flash_config_prepare_record(&record, cfg, sys_config.sequence + 1U);

    fmc_unlock();

    flash_clear_flags();
    if (fmc_page_erase(target_page) != FMC_READY) {
        fmc_lock();
        return FLASH_STORE_ERASE_FAILED;
    }
    flash_clear_flags();

    const uint8_t *bytes = (const uint8_t *)&record;
    uint32_t words = (sizeof(SystemConfig) + 3U) / 4U;

    for (uint32_t i = 0; i < words; i++) {
        uint32_t word = 0xFFFFFFFFUL;
        uint32_t offset = i * 4U;
        uint32_t remaining = sizeof(SystemConfig) - offset;
        if (remaining >= 4U) {
            memcpy(&word, &bytes[offset], 4U);
        } else {
            memcpy(&word, &bytes[offset], remaining);
        }

        if (fmc_word_program(target_page + offset, word) != FMC_READY) {
            fmc_lock();
            return FLASH_STORE_PROGRAM_FAILED;
        }
        flash_clear_flags();
    }

    fmc_lock();

    const SystemConfig *verify = (const SystemConfig *)target_page;
    if (!flash_config_record_valid(verify) || !flash_config_settings_equal(verify, &record)) {
        return FLASH_STORE_VERIFY_FAILED;
    }

    memcpy(&sys_config, verify, sizeof(SystemConfig));
    current_page_addr = target_page;
    return FLASH_STORE_OK;
#endif
}
