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

    ret = spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO);
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
        devcfg.queue_size     = 7;
        devcfg.command_bits  = 0;
        devcfg.dummy_bits    = 0;
    }

    esp_err_t ret = spi_bus_add_device(SPI3_HOST, &devcfg, p_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device fallo: %s", esp_err_to_name(ret));
        return SPP_ERROR;
    }

    call_count++;
    return SPP_OK;
}
//---End Init---

//---ESP32-specific message sender---
retval_t SPP_HAL_SPI_Transmit(void* handler, spp_uint8_t* p_data, spp_uint8_t length) {
    spi_device_handle_t p_handler = *(spi_device_handle_t*) handler;
    esp_err_t trans_result = ESP_OK;

    if (length <= 2) {
        // Only one transmission
        spi_transaction_t trans_desc = {0};
        trans_desc.length    = 8 * length;
        trans_desc.tx_buffer = p_data;
        trans_desc.rx_buffer = p_data;

        trans_result = spi_device_transmit(p_handler, &trans_desc);

    } else {        
        int num_ops = length / 2;
        spi_transaction_t transactions[num_ops];

        for (int i = 0; i < num_ops; i++) {
            spi_transaction_t* p_transaction = &transactions[i];
            *p_transaction = (spi_transaction_t){0}; //We put all the struct to zero
            p_transaction->length    = 16; 
            p_transaction->tx_buffer = &p_data[i * 2];
            p_transaction->rx_buffer = &p_data[i * 2];

            trans_result = spi_device_queue_trans(p_handler, p_transaction, portMAX_DELAY);
            if (trans_result != ESP_OK) break;
        }

        for (int i = 0; i < num_ops && trans_result == ESP_OK; i++) {
            spi_transaction_t* ret_t;
            trans_result = spi_device_get_trans_result(p_handler, &ret_t, portMAX_DELAY);
            if (trans_result != ESP_OK) break;
        }
    }

    if (trans_result != ESP_OK) {
        return SPP_ERROR_ON_SPI_TRANSACTION;
    }
    return SPP_OK;
}
