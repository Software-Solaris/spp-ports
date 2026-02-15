#include "spi.h" 
#include "core/types.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdint.h>
#include "macros_esp.h"

static const char *TAG = "SPP_HAL_SPI";

#define NUMBER_OF_DEVICES 2

static spi_device_handle_t spi_handler[NUMBER_OF_DEVICES]; 
static int device_state[NUMBER_OF_DEVICES] = {EMPTY};

//---Init---
retval_t SPP_HAL_SPI_BusInit(void)
{
    static spp_bool_t already_initialized = false;
    if (already_initialized == true){
        /** Allow repeated calls so multiple drivers can ensure the bus exists. */
        return SPP_OK;
    }
    already_initialized = true;
    esp_err_t ret;

    spi_bus_config_t buscfg = 
    {
    .miso_io_num     = MISO_PIN,
    .mosi_io_num     = MOSI_PIN,
    .sclk_io_num     = CLK_PIN,
    .quadwp_io_num   = -1,
    .quadhd_io_num   = -1,
    .max_transfer_sz = 0
    };

    ret = spi_bus_initialize(USED_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) return SPP_ERROR;

    return SPP_OK;
}

void* SPP_HAL_SPI_GetHandler(void)
{
    static spp_uint8_t i = 0;
    if (i >= NUMBER_OF_DEVICES){
        return NULL;
    }

    return (void*)&spi_handler[i++];
}

retval_t SPP_HAL_SPI_DeviceInit(void* p_handler)
{ 
    if (p_handler == NULL) return SPP_ERROR_NULL_POINTER;

    static uint8_t call_count = 0;   // Cuenta cuántas veces se ha llamado

    if (call_count >= 2) {
        ESP_LOGE(TAG, "SPI ya configurado (llamada extra)");
        return SPP_ERROR;
    }

    spi_device_handle_t *p_handle = (spi_device_handle_t*)p_handler;
    spi_device_interface_config_t devcfg = {0};

    if (call_count == 0) {   // 1ª llamada → ICM
        devcfg.clock_speed_hz = 1 * 1000 * 1000;
        devcfg.mode           = 0;
        devcfg.spics_io_num   = CS_PIN_ICM;
        devcfg.queue_size     = 20;
        devcfg.command_bits  = 0;
        devcfg.dummy_bits    = 0;   
    } 
    else {                   // 2ª llamada → BMP
        devcfg.clock_speed_hz = 500 * 1000;
        devcfg.mode           = 0;
        devcfg.spics_io_num   = CS_PIN_BMP;
        devcfg.queue_size     = 20;
        devcfg.command_bits  = 0;
        devcfg.dummy_bits    = 0;
    }

    {
        esp_err_t ret = spi_bus_add_device(USED_HOST, &devcfg, p_handle);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "spi_bus_add_device fallo: %s", esp_err_to_name(ret));
            return SPP_ERROR;
        }
    }

    call_count++;
    return SPP_OK;
}
//---End Init---

//---ESP32-specific message sender---
retval_t SPP_HAL_SPI_Transmit(void* handler, spp_uint8_t* p_data, spp_uint8_t length) {
    if ((handler == NULL) || (p_data == NULL) || (length == 0u)) {
        return SPP_ERROR_NULL_POINTER;
    }

    spi_device_handle_t p_handler = *(spi_device_handle_t*) handler;
    if (p_handler == NULL) {
        return SPP_ERROR_NULL_POINTER;
    }

    esp_err_t trans_result = ESP_OK;  

    int i = 0;
       
    while (i < length){
        spi_transaction_t trans_desc = { 0 };
        if (p_data[i] & 0x80 ) {
            /* Reading from registers */
            trans_desc.length    = 8 * 3;
            trans_desc.tx_buffer = &p_data[i];
            trans_desc.rx_buffer = &p_data[i];
            i += 3;
        } else {
            /* Writing to registers */
            trans_desc.length    = 8 * 2;
            trans_desc.tx_buffer = &p_data[i];
            i += 2;
        }
        trans_result = spi_device_transmit(p_handler, &trans_desc);
        if (trans_result != ESP_OK){
            return trans_result;
        }
    }
    return SPP_OK;
}
