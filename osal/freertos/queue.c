// Here goes the FreeRTOS function

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "osal/queue.h"
#include "core/types.h"
#include "core/returntypes.h"

/* typedef enum{
    SPP_OK,
    SPP_ERROR,
    SPP_NOT_ENOUGH_PACKETS,
    SPP_NULL_PACKET,
    SPP_ERROR_NULL_POINTER,
    SPP_ERROR_NOT_INITIALIZED,
    SPP_ERROR_INVALID_PARAMETER
}retval_t; */

// typedef void* osal_queue_handle_t;



retval_t SPP_OSAL_QueueCreate(osal_queue_handle_t queue_handle, uint32_t queue_length, uint32_t item_size)
{
    if (queue_handle == NULL) return SPP_ERROR_NULL_POINTER; // In case NULL is passed as argument
    if (queue_length == 0 || item_size == 0) return SPP_ERROR_INVALID_PARAMETER;

    QueueHandle_t rtos_handle = xQueueCreate(queue_length, item_size);

    if (rtos_handle == NULL) return SPP_ERROR_NULL_POINTER;

    *queue_handle = (osal_queue_handle_t)rtos_handle;
    return SPP_OK;
}






retval_t SPP_OSAL_QueueCreate(void* p_queue_handle, uint32_t queue_length, uint32_t item_size)
{
    if (queue_handle == NULL) return SPP_ERROR_NULL_POINTER; // In case NULL is passed as argument
    if (queue_length == 0 || item_size == 0) return SPP_ERROR_INVALID_PARAMETER;

    QueueHandle_t *p1_queue_handle = (QueueHandle_t*)p_queue_handle;

    QueueHandle_t queue_handle = *p_queue_handle;
    queue_handle = xQueueCreate(queue_length, item_size);

    if (rtos_handle == NULL) return SPP_ERROR_NULL_POINTER;

    *queue_handle = (void*)rtos_handle;
    return SPP_OK;
}



QueueHandle_t







retval_t SPP_OSAL_QueueCreate(void* queue_handle, uint32_t queue_length, uint32_t item_size)
{
    osal_queue_handle_t queue_handle;
    if (queue_handle == NULL) return SPP_ERROR_NULL_POINTER; // In case NULL is passed as argument
    if (queue_length == 0 || item_size == 0) return SPP_ERROR_INVALID_PARAMETER;

    QueueHandle_t rtos_handle = xQueueCreate(queue_length, item_size);

    if (rtos_handle == NULL) return SPP_ERROR_NULL_POINTER;

    queue_handle = (void*)rtos_handle;
    return SPP_OK;
}






/*  BaseType_t xQueueSendToBack(
 QueueHandle_t xQueue,
 const void * pvItemToQueue,
 TickType_t xTicksToWait
 ); */


retval_t SPP_OSAL_QueueSendToBack(osal_queue_handle_t queue_handle, uint32_t queue_length, uint32_t item_size)
{
    if (queue_handle == NULL) return SPP_ERROR_NULL_POINTER;0
}






