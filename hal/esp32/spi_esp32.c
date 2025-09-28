#include "spi.h" 
#include "esp_log.h" 
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>


static const char *TAG = "SPP_HAL_SPI";

//---Init---
typedef struct{
    spi_device_handle_t idf_handle;
    int bus_id;
}spp_spi_dev_esp32_t;

static spi_host_device_t map_bus_id_to_host(int bus_id)
{
#if SOC_SPI_PERIPH_NUM >= 3
    return (bus_id == 0) ? SPI2_HOST : SPI3_HOST;
#else
    return (bus_id == 0) ? HSPI_HOST : VSPI_HOST;
#endif
}

static retval_t ensure_bus_initialized(int bus_id, int pin_miso, int pin_mosi, int pin_sclk)
{
    static bool s_bus_inited[4] = {false,false,false,false};
    spi_host_device_t host = map_bus_id_to_host(bus_id);

    if (s_bus_inited[bus_id]) 
    {
        return SPP_OK;
    }

    spi_bus_config_t buscfg = {
        .mosi_io_num = pin_mosi,
        .miso_io_num = pin_miso,
        .sclk_io_num = pin_sclk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    esp_err_t ret = spi_bus_initialize(host, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize(bus=%d) err=%d", bus_id, ret);
        return SPP_ERROR;
    }
    s_bus_inited[bus_id] = true;
    return SPP_OK;
}

retval_t SPP_HAL_SPI_Init(void **out_handler, const SPP_SPI_InitCfg *cfg)
{
    if (cfg->queue_size == 0)
    {
        ESP_LOGW(TAG, "queue size = 0 ; adjusting to 1");
    }

    retval_t ret1;
    ret1 = ensure_bus_initialized(cfg->bus_id, cfg->pin_miso, cfg->pin_mosi, cfg->pin_sclk);
    if (ret1 != SPP_OK)
    {
        return SPP_ERROR;
    }

    spi_device_interface_config_t devcfg = {0};
    devcfg.clock_speed_hz = (int)cfg->max_hz;
    devcfg.mode           = (int)cfg->mode;        
    devcfg.spics_io_num   = cfg->pin_cs;          
    devcfg.queue_size     = (cfg->queue_size == 0) ? 1 : (int)cfg->queue_size;
    devcfg.command_bits   = 0;
    devcfg.address_bits   = 0;
    devcfg.dummy_bits     = 0;
    devcfg.flags = (cfg->duplex == SPP_SPI_HALF_DUPLEX) ? SPI_DEVICE_HALFDUPLEX : 0;

    spi_host_device_t host = map_bus_id_to_host(cfg->bus_id);
    spi_device_handle_t handle = NULL;
    esp_err_t ret = spi_bus_add_device(host, &devcfg, &handle);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "spi_bus_add_device(bus=%d) err=%d", cfg->bus_id, ret);
        return SPP_ERROR;
    }

    spp_spi_dev_esp32_t *dev = (spp_spi_dev_esp32_t *)calloc(1, sizeof(*dev));
    if (!dev) {
        ESP_LOGE(TAG, "calloc spp_spi_dev_esp32_t");
        (void)spi_bus_remove_device(handle);
        return SPP_ERROR;
    }

    dev->idf_handle = handle;
    dev->bus_id     = cfg->bus_id;

    *out_handler = (void *)dev;
    return SPP_OK;
}

//---End Init---

retval_t SPP_HAL_SPI_Transmit(void* handler, void* data_to_send, void* data_to_recieve, spp_uint8_t length) {
    spi_device_handle_t p_handler = (spi_device_handle_t) handler;
    esp_err_t trans_result = ESP_OK;

    if (length <= 2) {
        // Only one transmission
        spi_transaction_t trans_desc = {0};
        trans_desc.length    = 8 * length;
        trans_desc.tx_buffer = data_to_send;
        trans_desc.rx_buffer = data_to_recieve;

        trans_result = spi_device_transmit(p_handler, &trans_desc);

    } else {
        spp_uint8_t* p_tx = (spp_uint8_t*) data_to_send;
        spp_uint8_t* p_rx = (spp_uint8_t*) data_to_recieve;
        
        int num_ops = length / 2;
        spi_transaction_t transactions[num_ops];

        for (int i = 0; i < num_ops; i++) {
            spi_transaction_t* p_transaction = &transactions[i];
            *p_transaction = (spi_transaction_t){0}; //We put all the struct to zero
            p_transaction->length    = 16; 
            p_transaction->tx_buffer = &p_tx[i * 2];
            p_transaction->rx_buffer = &p_rx[i * 2];

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
        return SPP_ERROR;
    }
    return SPP_OK;
}


