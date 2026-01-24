// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/sdspi_host.html
// https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html
// https://github.com/espressif/esp-idf/blob/master/examples/storage/sd_card/sdspi/README.md (ejemplo premium)

#include "hal/storage/storage.h"
#include "core/types.h" 
#include "macros_esp.h"
#include "returntypes.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_err.h"

static spp_bool_t s_mounted = false;

retval_t SPP_HAL_Storage_Mount(void* p_cfg)
{
    if (s_mounted == true) return SPP_OK;

    const SPP_Storage_InitCfg* cfg = (const SPP_Storage_InitCfg*)p_cfg;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT(); // put predefined sdspi host config

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT(); // put predefined sdspi device config
    slot_config.gpio_cs = cfg->pin_cs;
    slot_config.host_id = host.slot;

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = (bool)cfg->format_if_mount_failed,
        .max_files = (int)cfg->max_files,
        .allocation_unit_size = (size_t)cfg->allocation_unit_size
    };
    // Not finished: next step esp_vfs_fat_sdspi_mount
}