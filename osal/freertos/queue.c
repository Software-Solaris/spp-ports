// Here goes the FreeRTOS function

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "osal/queue.h"
#include "core/types.h"
#include "core/returntypes.h"
#include "freertos/task.h"


void* SPP_OSAL_QueueCreate(uint32_t queue_length, uint32_t item_size)
{
    if (queue_length == 0 || item_size == 0) return NULL;

    QueueHandle_t p_queue_handle = xQueueCreate(queue_length, item_size);
    void* p_queue_handle_conv = (void*)p_queue_handle; // omitible no?

    if (p_queue_handle_conv == NULL) return NULL;

    return p_queue_handle_conv; // cambiar por: return (void*)p_queue_handle;
}


void* SPP_OSAL_QueueCreateStatic(uint32_t queue_length, uint32_t item_size, uint8_t* queue_storage, void* queue_buffer)
{
    if (queue_length == 0 || item_size == 0 || queue_buffer == 0) {
        return NULL;
    }

    if (item_size > 0 && queue_storage == NULL) { // si item_size > 0, necesitamos storage válido
        return NULL;
    }

    QueueHandle_t queue_handle = xQueueCreateStatic(queue_length, item_size, queue_storage, (void*)queue_buffer);

    if (queue_handle == NULL) return NULL;

    return (void*)queue_handle;
}


uint32_t SPP_OSAL_QueueMessagesWaiting(void* queue_handle)
{
    if (queue_handle == NULL) return 0;

    QueueHandle_t rtos_handle = (QueueHandle_t)queue_handle;
    uint32_t queued_items = (uint32_t)uxQueueMessagesWaiting(rtos_handle);

    return queued_items;
}


// Functions added for datapool
static TickType_t spp_osal_ms_to_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == 0u) return 0u;

    TickType_t ticks = pdMS_TO_TICKS(timeout_ms);
    if (ticks == 0u) ticks = 1u; // evita redondeo a 0
    return ticks;
}

retval_t SPP_OSAL_QueueSend(void* queue_handle, const void* p_item, uint32_t timeout_ms)
{
    retval_t ret = SPP_OK;

    if (queue_handle == NULL || p_item == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)queue_handle;
    TickType_t ticks = spp_osal_ms_to_ticks(timeout_ms);

    if (xQueueSend(q, p_item, ticks) != pdTRUE)
    {
        ret = SPP_ERROR;
        return ret;
    }

    return ret;
}

retval_t SPP_OSAL_QueueReceive(void* queue_handle, void* p_out_item, uint32_t timeout_ms)
{
    retval_t ret = SPP_OK;

    if (queue_handle == NULL || p_out_item == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)queue_handle;
    TickType_t ticks = spp_osal_ms_to_ticks(timeout_ms);

    if (xQueueReceive(q, p_out_item, ticks) != pdTRUE)
    {
        // Para datapool: no había punteros disponibles en el tiempo dado
        ret = SPP_NOT_ENOUGH_PACKETS;
        return ret;
    }

    return ret;
}

retval_t SPP_OSAL_QueueReset(void* queue_handle)
{
    retval_t ret = SPP_OK;

    if (queue_handle == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)queue_handle;

    if (xQueueReset(q) != pdTRUE)
    {
        ret = SPP_ERROR;
        return ret;
    }

    return ret;
}