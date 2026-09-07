#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

typedef enum {
    SOLDER_LOW_TEMP = 0,
    SOLDER_MED_TEMP,
    SOLDER_HIGH_TEMP
} SolderType;

typedef struct {
    uint32_t magic;         // To check if flash is formatted
    uint32_t version;       // Record layout version
    float target_temp;      // Default const temp target
    float pid_kp;
    float pid_ki;
    float pid_kd;
    SolderType solder_type; // 0=Low, 1=Med, 2=High
    uint32_t sequence;      // For ping-pong buffering
    uint32_t checksum;      // FNV-1a over this struct with checksum cleared
} SystemConfig;

typedef enum {
    FLASH_STORE_OK = 0,
    FLASH_STORE_NO_CHANGE,
    FLASH_STORE_INVALID_CONFIG,
    FLASH_STORE_ERASE_FAILED,
    FLASH_STORE_PROGRAM_FAILED,
    FLASH_STORE_VERIFY_FAILED
} FlashStoreResult;

extern SystemConfig sys_config;

void flash_store_init(void);
FlashStoreResult flash_store_save(const SystemConfig *cfg);

void flash_config_load_defaults(SystemConfig *cfg);
bool flash_config_settings_valid(const SystemConfig *cfg);
bool flash_config_record_valid(const SystemConfig *cfg);
bool flash_config_settings_equal(const SystemConfig *a, const SystemConfig *b);
void flash_config_prepare_record(SystemConfig *dst, const SystemConfig *settings, uint32_t sequence);
const SystemConfig *flash_config_select_latest(const SystemConfig *a, const SystemConfig *b);
uint32_t flash_config_checksum(const SystemConfig *cfg);

#endif // FLASH_STORE_H
