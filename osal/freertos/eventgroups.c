/**
 * @file eventgroups.c
 * @brief FreeRTOS OSAL event groups implementation for the SPP framework.
 *
 * Provides static event group creation, ISR-safe bit setting, and blocking
 * bit wait operations using FreeRTOS event group primitives.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "spp/osal/eventgroups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "spp/core/returntypes.h"
#include "spp/core/types.h"
#include "spp/core/macros.h"
#include "macros_freertos.h"

/* ============================================================================
 * Private Variables
 * ========================================================================= */

/** @brief Static storage for FreeRTOS event group buffers. */
static StaticEventGroup_t s_eventGroupBuffers[NUM_EVENT_GROUPS];

/** @brief Number of event group buffers currently allocated. */
static spp_uint8_t s_counter = 0;

/* ============================================================================
 * Public Functions
 * ========================================================================= */

/**
 * @brief Allocate an event group buffer from the static pool.
 *
 * Each call returns the next available StaticEventGroup_t from
 * s_eventGroupBuffers.
 *
 * @return Pointer to the allocated buffer, or NULL if the pool is exhausted.
 */
void *SPP_OSAL_GetEventGroupsBuffer()
{
    if (s_counter >= NUM_EVENT_GROUPS)
    {
        return NULL;
    }
    StaticEventGroup_t *p_buffer = &s_eventGroupBuffers[s_counter];
    s_counter += 1;
    void *p_bufferEventGroup = (void *)p_buffer;
    return p_bufferEventGroup;
}

/**
 * @brief Create a new event group.
 *
 * In static mode (STATIC defined), uses xEventGroupCreateStatic with the
 * provided buffer. In dynamic mode, the buffer is ignored and
 * xEventGroupCreate is used instead.
 *
 * @param[in] p_eventGroupBuffer Pointer to a StaticEventGroup_t buffer
 *                               (used only in static allocation mode).
 * @return Event group handle as void pointer, or NULL on failure.
 */
void *SPP_OSAL_EventGroupCreate(void *p_eventGroupBuffer)
{
#ifdef STATIC
    EventGroupHandle_t eg = xEventGroupCreateStatic((StaticEventGroup_t *)p_eventGroupBuffer);
#else
    (void)p_eventGroupBuffer; /* Unused in dynamic mode */
    EventGroupHandle_t eg = xEventGroupCreate();
#endif

    if (eg == NULL)
        return NULL;
    return (void *)eg;
}

/**
 * @brief Set bits in an event group from ISR context.
 *
 * Wraps xEventGroupSetBitsFromISR and converts the FreeRTOS result to
 * SPP return types.
 *
 * @param[in]  p_eventGroup             Event group handle.
 * @param[in]  bits_to_set              Bits to set in the event group.
 * @param[out] p_previousBits           Receives previous bit values (set to 0;
 *                                      not fully supported by FreeRTOS ISR API).
 * @param[out] p_higherPriorityTaskWoken Set to 1 if a higher-priority task was
 *                                      woken, 0 otherwise.
 * @return SPP_OK on success, SPP_ERROR if the set operation failed.
 */
retval_t OSAL_EventGroupSetBitsFromISR(void *p_eventGroup, osal_eventbits_t bits_to_set,
                                       osal_eventbits_t *p_previousBits,
                                       spp_uint8_t *p_higherPriorityTaskWoken)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)p_eventGroup;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result =
        xEventGroupSetBitsFromISR(eg, (EventBits_t)bits_to_set, &xHigherPriorityTaskWoken);

    if (p_previousBits != NULL)
    {
        *p_previousBits = 0;
    }

    if (p_higherPriorityTaskWoken != NULL)
    {
        if (xHigherPriorityTaskWoken == pdTRUE)
        {
            *p_higherPriorityTaskWoken = 1;
        }
        else
        {
            *p_higherPriorityTaskWoken = 0;
        }
    }

    if (result == pdPASS)
    {
        return SPP_OK;
    }

    return SPP_ERROR;
}

/**
 * @brief Wait for bits to be set in an event group.
 *
 * Blocks the calling task until the requested bits are set or the timeout
 * expires. Supports both wait-for-all and wait-for-any semantics.
 *
 * @param[in]  p_eventGroup     Event group handle.
 * @param[in]  bits_to_wait     Bit mask to wait on.
 * @param[in]  clear_on_exit    Non-zero to clear matched bits on return.
 * @param[in]  wait_for_all_bits Non-zero to require all bits, zero for any.
 * @param[in]  timeout_ms       Maximum wait time in milliseconds (0 = no wait).
 * @param[out] p_actualBits     Receives the actual event bits at return time
 *                              (may be NULL).
 * @return SPP_OK if the requested bits were set, SPP_ERROR on timeout.
 */
retval_t OSAL_EventGroupWaitBits(void *p_eventGroup, osal_eventbits_t bits_to_wait,
                                 spp_uint8_t clear_on_exit, spp_uint8_t wait_for_all_bits,
                                 spp_uint32_t timeout_ms, osal_eventbits_t *p_actualBits)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)p_eventGroup;
    TickType_t timeoutTicks;
    BaseType_t waitAll;
    BaseType_t clearOnExitFlag;
    EventBits_t result;

    if (timeout_ms == 0)
    {
        timeoutTicks = 0;
    }
    else
    {
        timeoutTicks = pdMS_TO_TICKS(timeout_ms);
    }

    if (wait_for_all_bits != 0)
    {
        waitAll = pdTRUE;
    }
    else
    {
        waitAll = pdFALSE;
    }

    if (clear_on_exit != 0)
    {
        clearOnExitFlag = pdTRUE;
    }
    else
    {
        clearOnExitFlag = pdFALSE;
    }

    result =
        xEventGroupWaitBits(eg, (EventBits_t)bits_to_wait, clearOnExitFlag, waitAll, timeoutTicks);

    if (p_actualBits != NULL)
    {
        *p_actualBits = (osal_eventbits_t)result;
    }

    if (wait_for_all_bits != 0)
    {
        if ((result & bits_to_wait) == bits_to_wait)
        {
            return SPP_OK;
        }
    }
    else
    {
        if ((result & bits_to_wait) != 0)
        {
            return SPP_OK;
        }
    }

    return SPP_ERROR;
}
