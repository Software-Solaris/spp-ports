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

    QueueHandle_t queue_handle = xQueueCreateStatic(queue_length, item_size, queue_storage, (StaticQueue_t*)queue_buffer);

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
