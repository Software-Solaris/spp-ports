/**
 * @file spi_esp32.c
 * @brief ESP32 SPI HAL implementation for the SPP framework.
 *
 * Provides SPI bus initialization, device registration, and full-duplex
 * transmit/receive for the ICM20948 IMU and BMP390 barometer on SPI2_HOST.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "spp/hal/spi/spi.h"
#include "spp/core/types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdint.h>
#include "macros_esp.h"

/* ============================================================================
 * Private Constants
 * ========================================================================= */

/** @brief Number of SPI devices managed by this driver. */
#define K_NUMBER_OF_DEVICES 2

/* ============================================================================
 * Private Variables
 * ========================================================================= */

/** @brief Logging tag for ESP-IDF log macros. */
static const char *TAG = "SPP_HAL_SPI";

/** @brief Cached pointer to the BMP390 SPI handler for read-offset logic. */
static void *s_pBmpHandler;

/** @brief SPI device handles for each registered device. */
static spi_device_handle_t s_spiHandler[K_NUMBER_OF_DEVICES];

/** @brief Tracks initialization state of each device slot. */
static int s_deviceState[K_NUMBER_OF_DEVICES] = {EMPTY};

/* ============================================================================
 * Public Functions — Bus Initialization
 * ========================================================================= */

/**
 * @brief Initialize the shared SPI bus.
 *
 * Configures SPI2_HOST with the MISO, MOSI, and CLK pins defined in
 * macros_esp.h. Safe to call multiple times; subsequent calls return SPP_OK
 * without re-initializing.
 *
 * @return SPP_OK on success, SPP_ERROR on SPI bus init failure.
 */
retval_t SPP_HAL_SPI_BusInit(void)
{
    static spp_bool_t alreadyInitialized = false;
    if (alreadyInitialized == true)
    {
        /** Allow repeated calls so multiple drivers can ensure the bus exists. */
        return SPP_OK;
    }
    alreadyInitialized = true;
    esp_err_t ret;

    spi_bus_config_t buscfg = {.miso_io_num = MISO_PIN,
                               .mosi_io_num = MOSI_PIN,
                               .sclk_io_num = CLK_PIN,
                               .quadwp_io_num = -1,
                               .quadhd_io_num = -1,
                               .max_transfer_sz = 0};

    ret = spi_bus_initialize(USED_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
        return SPP_ERROR;

    return SPP_OK;
}

/* ============================================================================
 * Public Functions — Device Handle Management
 * ========================================================================= */

/**
 * @brief Get the next available SPI device handler.
 *
 * Returns a pointer to an internal spi_device_handle_t slot. Called
 * sequentially: first call returns the ICM slot, second returns the BMP slot.
 *
 * @return Pointer to the next available handler, or NULL if all slots are used.
 */
void *SPP_HAL_SPI_GetHandler(void)
{
    static spp_uint8_t i = 0;
    if (i >= K_NUMBER_OF_DEVICES)
    {
        return NULL;
    }

    return (void *)&s_spiHandler[i++];
}

/* ============================================================================
 * Public Functions — Device Initialization
 * ========================================================================= */

/**
 * @brief Initialize an SPI device on the shared bus.
 *
 * First call configures the ICM20948 (CS GPIO 21, 1 MHz). Second call
 * configures the BMP390 (CS GPIO 18, 500 kHz). Additional calls return
 * SPP_ERROR.
 *
 * @param[in] p_handler Pointer to the spi_device_handle_t obtained from
 *                      SPP_HAL_SPI_GetHandler().
 * @return SPP_OK on success, SPP_ERROR_NULL_POINTER if p_handler is NULL,
 *         SPP_ERROR if device limit exceeded or bus add fails.
 */
retval_t SPP_HAL_SPI_DeviceInit(void *p_handler)
{
    if (p_handler == NULL)
        return SPP_ERROR_NULL_POINTER;

    static uint8_t callCount = 0; /* Tracks how many times this function has been called */

    if (callCount >= 2)
    {
        ESP_LOGE(TAG, "SPI already configured (extra call)");
        return SPP_ERROR;
    }

    spi_device_handle_t *p_handle = (spi_device_handle_t *)p_handler;
    spi_device_interface_config_t devcfg = {0};

    if (callCount == 0)
    { /* First call: ICM20948 */
        devcfg.clock_speed_hz = 1 * 1000 * 1000;
        devcfg.mode = 0;
        devcfg.spics_io_num = CS_PIN_ICM;
        devcfg.queue_size = 20;
        devcfg.command_bits = 0;
        devcfg.dummy_bits = 0;
    }
    else
    { /* Second call: BMP390 */
        devcfg.clock_speed_hz = 500 * 1000;
        devcfg.mode = 0;
        devcfg.spics_io_num = CS_PIN_BMP;
        devcfg.queue_size = 20;
        devcfg.command_bits = 0;
        devcfg.dummy_bits = 0;
        s_pBmpHandler = p_handler;
    }

    {
        esp_err_t ret = spi_bus_add_device(USED_HOST, &devcfg, p_handle);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "spi_bus_add_device failed: %s", esp_err_to_name(ret));
            return SPP_ERROR;
        }
    }

    callCount++;
    return SPP_OK;
}

/* ============================================================================
 * Public Functions — SPI Transmit / Receive
 * ========================================================================= */

/**
 * @brief Perform a full-duplex SPI transaction.
 *
 * Detects read vs write by the MSB of p_data[0]. For reads, the received
 * bytes are copied back into p_data with the appropriate offset (ICM: data
 * starts at byte 1, BMP: data starts at byte 2 due to dummy byte).
 *
 * @param[in]     p_handler Pointer to the spi_device_handle_t for the target device.
 * @param[in,out] p_data    TX buffer on entry; RX data overwritten on read operations.
 * @param[in]     length    Total number of bytes in the transaction.
 * @return SPP_OK on success, SPP_ERROR_NULL_POINTER on invalid args,
 *         or the ESP-IDF error cast to retval_t on SPI failure.
 */
retval_t SPP_HAL_SPI_Transmit(void *p_handler, spp_uint8_t *p_data, spp_uint8_t length)
{
    if ((p_handler == NULL) || (p_data == NULL) || (length == 0u))
    {
        return SPP_ERROR_NULL_POINTER;
    }

    spi_device_handle_t spiHandle = *(spi_device_handle_t *)p_handler;
    if (spiHandle == NULL)
    {
        return SPP_ERROR_NULL_POINTER;
    }

    spi_transaction_t transDesc = {0};
    esp_err_t transResult;

    if (p_data[0] & 0x80)
    {
        /* Reading: one single SPI transaction (CS held low for all bytes).
         * ICM: byte 0 = command, byte 1+ = data.
         * BMP: byte 0 = command, byte 1 = dummy, byte 2+ = data.
         * In both cases the whole buffer goes in one burst so that burst
         * reads (e.g. 43-byte FIFO read) work correctly. */
        spp_uint8_t rxBuf[64] = {0};

        transDesc.length = 8 * length;
        transDesc.tx_buffer = p_data;
        transDesc.rx_buffer = rxBuf;

        transResult = spi_device_transmit(spiHandle, &transDesc);
        if (transResult != ESP_OK)
            return (retval_t)transResult;

        /* Copy received bytes back.
         * BMP needs to skip 1 extra dummy byte (data starts at [2]).
         * ICM data starts at [1]. */
        spp_uint8_t dataStart = (p_handler == s_pBmpHandler) ? 2u : 1u;
        for (spp_uint8_t i = dataStart; i < length; i++)
        {
            p_data[i] = rxBuf[i];
        }
    }
    else
    {
        /* Writing: one single SPI transaction. */
        transDesc.length = 8 * length;
        transDesc.tx_buffer = p_data;

        transResult = spi_device_transmit(spiHandle, &transDesc);
        if (transResult != ESP_OK)
            return (retval_t)transResult;
    }

    return SPP_OK;
}
