// Here goes the FreeRTOS function

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "osal/queue.h"
#include "core/types.h"
#include "core/returntypes.h"


void* SPP_OSAL_QueueCreate(uint32_t queue_length, uint32_t item_size)
{
    if (queue_length == 0 || item_size == 0) return NULL;

    QueueHandle_t p_queue_handle = xQueueCreate(queue_length, item_size);
    void* p_queue_handle_conv = (void*)p_queue_handle; // omitible no?

    if (p_queue_handle_conv == NULL) return NULL;

    return p_queue_handle_conv; // cambiar por: return (void*)p_queue_handle;
}



//--------------FASE DE PRUEBAS--------------------


/**
 * @brief Obtiene el número de elementos en cola
 * 
 * @param queue_handle Handle de la cola
 * @return uint32_t Número de elementos en cola
 */
uint32_t SPP_OSAL_QueueMessagesWaiting(void* queue_handle)
{
    if (queue_handle == NULL) return 0;

    QueueHandle_t rtos_handle = (QueueHandle_t)queue_handle;
    uint32_t queued_items = (uint32_t)uxQueueMessagesWaiting(rtos_handle);

    return queued_items;
}


//--------------BORRADORES--------------------













