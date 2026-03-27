/**
 * @file storage.c
 * @brief ESP32 SD card storage HAL implementation for the SPP framework.
 *
 * Wraps ESP-IDF FATFS and SDSPI APIs to provide mount/unmount functionality
 * for SD card access via the SPP storage abstraction.
 *
 * @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/sdspi_host.html
 * @see https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "spp/hal/storage/storage.h"
#include "spp/core/types.h"
#include "spp/core/returntypes.h"
#include "macros_esp.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "esp_err.h"

/* ============================================================================
 * Private Variables
 * ========================================================================= */

/** @brief Tracks whether the SD card filesystem is currently mounted. */
static spp_bool_t s_mounted = false;

/** @brief Pointer to the SD/MMC card descriptor obtained during mount. */
static sdmmc_card_t *s_card = NULL;

/* ============================================================================
 * Public Functions
 * ========================================================================= */

/**
 * @brief Mount the SD card filesystem.
 *
 * Initializes the SDSPI host and mounts a FAT filesystem using the
 * configuration provided in p_cfg. Safe to call multiple times; returns
 * SPP_OK immediately if already mounted.
 *
 * @param[in] p_cfg Pointer to an SPP_Storage_InitCfg structure with mount
 *                  parameters (base path, CS pin, host ID, format options).
 * @return SPP_OK on success, SPP_ERROR on mount failure.
 */
retval_t SPP_HAL_Storage_Mount(void *p_cfg)
{
    if (s_mounted == true)
    {
        return SPP_OK;
    }

    const SPP_Storage_InitCfg *p_initCfg = (const SPP_Storage_InitCfg *)p_cfg;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT(); /* Default SDSPI host config */

    sdspi_device_config_t slotConfig =
        SDSPI_DEVICE_CONFIG_DEFAULT(); /* Default SDSPI device config */
    slotConfig.gpio_cs = p_initCfg->pin_cs;
    slotConfig.host_id = (spi_host_device_t)p_initCfg->spi_host_id;

    esp_vfs_fat_mount_config_t mountConfig = {
        .format_if_mount_failed = (bool)p_initCfg->format_if_mount_failed,
        .max_files = (int)p_initCfg->max_files,
        .allocation_unit_size = (size_t)p_initCfg->allocation_unit_size};

    esp_err_t ret;
    ret = esp_vfs_fat_sdspi_mount(p_initCfg->p_base_path, &host, &slotConfig, &mountConfig, &s_card);

    if (ret != ESP_OK)
    {
        s_card = NULL; /* If mount failed, s_card could be undefined */
        return SPP_ERROR;
    }

    s_mounted = true;

    return SPP_OK;
}

/**
 * @brief Unmount the SD card filesystem.
 *
 * Unmounts the FAT filesystem and releases the SD card resources. Safe to
 * call when not mounted; returns SPP_OK immediately.
 *
 * @param[in] p_cfg Pointer to an SPP_Storage_InitCfg structure (base_path
 *                  is used for the unmount call).
 * @return SPP_OK on success, SPP_ERROR on unmount failure.
 */
retval_t SPP_HAL_Storage_Unmount(void *p_cfg)
{
    if (s_mounted == false)
    {
        return SPP_OK;
    }

    const SPP_Storage_InitCfg *p_initCfg = (const SPP_Storage_InitCfg *)p_cfg;

    esp_err_t ret;
    ret = esp_vfs_fat_sdcard_unmount(p_initCfg->p_base_path, s_card);

    if (ret != ESP_OK)
    {
        s_card = NULL;     /* If unmount failed, s_card could be undefined */
        s_mounted = false; /* Consider it unmounted anyway to avoid stuck state */
        return SPP_ERROR;
    }

    s_mounted = false;
    s_card = NULL;

    return SPP_OK;
}
