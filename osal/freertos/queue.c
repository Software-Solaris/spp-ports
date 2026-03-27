/**
 * @file queue.c
 * @brief FreeRTOS OSAL queue implementation for the SPP framework.
 *
 * Wraps FreeRTOS queue APIs (dynamic and static creation, send, receive,
 * reset, and message count) behind the SPP OSAL queue interface.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "spp/osal/queue.h"
#include "spp/core/types.h"
#include "spp/core/returntypes.h"
#include "freertos/task.h"

/* ============================================================================
 * Private Functions
 * ========================================================================= */

/**
 * @brief Convert a millisecond timeout to FreeRTOS ticks.
 *
 * Ensures that a non-zero millisecond value always produces at least 1 tick,
 * avoiding silent rounding to zero.
 *
 * @param[in] timeoutMs Timeout in milliseconds.
 * @return Equivalent TickType_t value.
 */
static TickType_t spp_osal_ms_to_ticks(uint32_t timeoutMs)
{
    if (timeoutMs == 0u)
        return 0u;

    TickType_t ticks = pdMS_TO_TICKS(timeoutMs);
    if (ticks == 0u)
        ticks = 1u; /* Avoid rounding to 0 */
    return ticks;
}

/* ============================================================================
 * Public Functions — Queue Creation
 * ========================================================================= */

/**
 * @brief Create a new queue using dynamic memory allocation.
 *
 * @param[in] queue_length Maximum number of items the queue can hold.
 * @param[in] item_size    Size of each item in bytes.
 * @return Queue handle as void pointer, or NULL on failure.
 */
void *SPP_OSAL_QueueCreate(uint32_t queue_length, uint32_t item_size)
{
    if (queue_length == 0 || item_size == 0)
        return NULL;

    QueueHandle_t queueHandle = xQueueCreate(queue_length, item_size);

    if (queueHandle == NULL)
        return NULL;

    return (void *)queueHandle;
}

/**
 * @brief Create a new queue using static memory allocation.
 *
 * @param[in] queue_length  Maximum number of items the queue can hold.
 * @param[in] item_size     Size of each item in bytes.
 * @param[in] p_queueStorage Pointer to the static storage area for queue items.
 * @param[in] p_queueBuffer  Pointer to the StaticQueue_t buffer.
 * @return Queue handle as void pointer, or NULL on failure.
 */
void *SPP_OSAL_QueueCreateStatic(uint32_t queue_length, uint32_t item_size, uint8_t *p_queueStorage,
                                 void *p_queueBuffer)
{
    if (queue_length == 0 || item_size == 0 || p_queueBuffer == 0)
    {
        return NULL;
    }

    if (item_size > 0 && p_queueStorage == NULL)
    { /* If item_size > 0, valid storage is required */
        return NULL;
    }

    QueueHandle_t queueHandle =
        xQueueCreateStatic(queue_length, item_size, p_queueStorage, (void *)p_queueBuffer);

    if (queueHandle == NULL)
        return NULL;

    return (void *)queueHandle;
}

/* ============================================================================
 * Public Functions — Queue Status
 * ========================================================================= */

/**
 * @brief Get the number of messages currently waiting in a queue.
 *
 * @param[in] p_queueHandle Queue handle.
 * @return Number of queued items, or 0 if the handle is NULL.
 */
uint32_t SPP_OSAL_QueueMessagesWaiting(void *p_queueHandle)
{
    if (p_queueHandle == NULL)
        return 0;

    QueueHandle_t rtosHandle = (QueueHandle_t)p_queueHandle;
    uint32_t queuedItems = (uint32_t)uxQueueMessagesWaiting(rtosHandle);

    return queuedItems;
}

/* ============================================================================
 * Public Functions — Queue Send / Receive / Reset
 * ========================================================================= */

/**
 * @brief Send an item to a queue.
 *
 * @param[in] p_queueHandle Queue handle.
 * @param[in] p_item        Pointer to the item to enqueue.
 * @param[in] timeout_ms    Maximum wait time in milliseconds.
 * @return SPP_OK on success, SPP_ERROR_NULL_POINTER if handles are NULL,
 *         SPP_ERROR if the send timed out.
 */
retval_t SPP_OSAL_QueueSend(void *p_queueHandle, const void *p_item, uint32_t timeout_ms)
{
    retval_t ret = SPP_OK;

    if (p_queueHandle == NULL || p_item == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)p_queueHandle;
    TickType_t ticks = spp_osal_ms_to_ticks(timeout_ms);

    if (xQueueSend(q, p_item, ticks) != pdTRUE)
    {
        ret = SPP_ERROR;
        return ret;
    }

    return ret;
}

/**
 * @brief Receive an item from a queue.
 *
 * @param[in]  p_queueHandle Queue handle.
 * @param[out] p_outItem     Pointer to the buffer that receives the dequeued item.
 * @param[in]  timeout_ms    Maximum wait time in milliseconds.
 * @return SPP_OK on success, SPP_ERROR_NULL_POINTER if handles are NULL,
 *         SPP_NOT_ENOUGH_PACKETS if no item was available within the timeout.
 */
retval_t SPP_OSAL_QueueReceive(void *p_queueHandle, void *p_outItem, uint32_t timeout_ms)
{
    retval_t ret = SPP_OK;

    if (p_queueHandle == NULL || p_outItem == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)p_queueHandle;
    TickType_t ticks = spp_osal_ms_to_ticks(timeout_ms);

    if (xQueueReceive(q, p_outItem, ticks) != pdTRUE)
    {
        /* For datapool: no pointers were available within the given time */
        ret = SPP_NOT_ENOUGH_PACKETS;
        return ret;
    }

    return ret;
}

/**
 * @brief Reset a queue to its empty state.
 *
 * @param[in] p_queueHandle Queue handle.
 * @return SPP_OK on success, SPP_ERROR_NULL_POINTER if the handle is NULL,
 *         SPP_ERROR if the reset failed.
 */
retval_t SPP_OSAL_QueueReset(void *p_queueHandle)
{
    retval_t ret = SPP_OK;

    if (p_queueHandle == NULL)
    {
        ret = SPP_ERROR_NULL_POINTER;
        return ret;
    }

    QueueHandle_t q = (QueueHandle_t)p_queueHandle;

    if (xQueueReset(q) != pdTRUE)
    {
        ret = SPP_ERROR;
        return ret;
    }

    return ret;
}
