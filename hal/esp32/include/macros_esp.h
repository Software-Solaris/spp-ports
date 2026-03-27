/**
 * @file macros_esp.h
 * @brief ESP32 HAL platform-specific pin definitions and constants.
 */

#ifndef MACROS_ESP_H
#define MACROS_ESP_H

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "spp/hal/spi/spi.h"
#include "esp_log.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include <string.h>

/* ============================================================================
 * SPI Pin Definitions
 * ========================================================================= */

/** @brief MISO (CIPO) GPIO pin number. */
#define MISO_PIN 47

/** @brief MOSI (COPI) GPIO pin number. */
#define MOSI_PIN 38

/** @brief SPI clock GPIO pin number. */
#define CLK_PIN 48

/** @brief SPI host peripheral to use. */
#define USED_HOST SPI2_HOST

/* ============================================================================
 * Chip Select Pin Definitions
 * ========================================================================= */

/** @brief BMP390 barometer chip select GPIO pin. */
#define CS_PIN_BMP 18

/** @brief ICM20948 IMU chip select GPIO pin. */
#define CS_PIN_ICM 21

/** @brief SD card chip select GPIO pin. */
#define CS_PIN_SDC 5

/** @brief Maximum number of SPI devices supported. */
#define MAX_DEVICES 4

/* ============================================================================
 * Device State Enumeration
 * ========================================================================= */

/** @brief SPI device state values. */
enum
{
    EMPTY = 0, /**< Device slot is unoccupied. */
    READY = 1  /**< Device slot is initialized and ready. */
};

#endif /* MACROS_ESP_H */
