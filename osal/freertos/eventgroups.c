/**
 * @file eventgroups.c
 * @brief OSAL Event Groups FreeRTOS Implementation
 * @version 1.0.0
 * @date 2024
 * 
 * This file provides the FreeRTOS implementation of event groups functions.
 */

#include "osal/eventgroups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "core/returntypes.h"
#include "core/types.h"
#include "core/macros.h"
#include "macros_freertos.h"

StaticEventGroup_t xEventGroupBuffers[NUM_EVENT_GROUPS];

static spp_uint8_t counter = 0;

void* SPP_OSAL_GetEventGroupsBuffer(){
    if (counter >= NUM_EVENT_GROUPS){
        return NULL;
    }
    StaticEventGroup_t *p_buffer = &xEventGroupBuffers[counter];
    counter += 1;
    void* p_buffer_event_group = (void*)p_buffer;
    return p_buffer_event_group;
}

/**
 * @brief Create a new event group
 */
void* SPP_OSAL_EventGroupCreate(void* event_group_buffer)
{
#ifdef STATIC
    EventGroupHandle_t eg = xEventGroupCreateStatic((StaticEventGroup_t*)event_group_buffer);
#else
    (void)event_group_buffer; // no se usa en modo dinámico
    EventGroupHandle_t eg = xEventGroupCreate();
#endif

    if (eg == NULL) return NULL;
    return (void*)eg;
}

retval_t OSAL_EventGroupSetBitsFromISR(void* event_group,
                                      osal_eventbits_t bits_to_set,
                                      osal_eventbits_t* previous_bits,
                                      spp_uint8_t* higher_priority_task_woken)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)event_group;

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = xEventGroupSetBitsFromISR(
        eg,
        (EventBits_t)bits_to_set,
        &xHigherPriorityTaskWoken
    );

    if (previous_bits != NULL) {
        *previous_bits = 0;
    }

    if (higher_priority_task_woken != NULL) {
        if (xHigherPriorityTaskWoken == pdTRUE) {
            *higher_priority_task_woken = 1;
        } else {
            *higher_priority_task_woken = 0;
        }
    }

    if (result == pdPASS) {
        return SPP_OK;
    }

    return SPP_ERROR;
}

retval_t OSAL_EventGroupWaitBits(void* event_group,
                                 osal_eventbits_t bits_to_wait,
                                 spp_uint8_t clear_on_exit,
                                 spp_uint8_t wait_for_all_bits,
                                 spp_uint32_t timeout_ms,
                                 osal_eventbits_t* actual_bits)
{
    EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
    TickType_t timeout_ticks;
    BaseType_t wait_all;
    BaseType_t clear_on_exit_flag;
    EventBits_t result;

    if (timeout_ms == 0) {
        timeout_ticks = 0;
    } else {
        timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    }

    if (wait_for_all_bits != 0) {
        wait_all = pdTRUE;
    } else {
        wait_all = pdFALSE;
    }

    if (clear_on_exit != 0) {
        clear_on_exit_flag = pdTRUE;
    } else {
        clear_on_exit_flag = pdFALSE;
    }

    result = xEventGroupWaitBits(
        eg,
        (EventBits_t)bits_to_wait,
        clear_on_exit_flag,
        wait_all,
        timeout_ticks
    );

    if (actual_bits != NULL) {
        *actual_bits = (osal_eventbits_t)result;
    }

    if (wait_for_all_bits != 0) {
        if ((result & bits_to_wait) == bits_to_wait) {
            return SPP_OK;
        }
    } else {
        if ((result & bits_to_wait) != 0) {
            return SPP_OK;
        }
    }

    return SPP_ERROR;
}


// /**
//  * @brief Set bits in an event group
//  */
// retval_t OSAL_EventGroupSetBits(osal_eventgroup_handle_t event_group, 
//                                osal_eventbits_t bits_to_set,
//                                osal_eventbits_t* previous_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     EventBits_t previous = xEventGroupSetBits(eg, bits_to_set);
    
//     if (previous_bits != NULL) {
//         *previous_bits = previous;
//     }
    
//     return SPP_OK;
// }

// /**
//  * @brief Set bits in an event group from ISR
//  */
// retval_t OSAL_EventGroupSetBitsFromISR(osal_eventgroup_handle_t event_group,
//                                       osal_eventbits_t bits_to_set,
//                                       osal_eventbits_t* previous_bits,
//                                       spp_uint8_t* higher_priority_task_woken)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     BaseType_t result = xEventGroupSetBitsFromISR(eg, bits_to_set, &xHigherPriorityTaskWoken);
    
//     if (previous_bits != NULL) {
//         *previous_bits = result;
//     }
    
//     if (higher_priority_task_woken != NULL) {
//         *higher_priority_task_woken = (xHigherPriorityTaskWoken == pdTRUE) ? 1 : 0;
//     }
    
//     return SPP_OK;
// }

// /**
//  * @brief Wait for bits to be set in an event group
//  */
// retval_t OSAL_EventGroupWaitBits(osal_eventgroup_handle_t event_group,
//                                 osal_eventbits_t bits_to_wait,
//                                 spp_uint8_t clear_on_exit,
//                                 spp_uint8_t wait_for_all_bits,
//                                 spp_uint32_t timeout_ms,
//                                 osal_eventbits_t* actual_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     if (actual_bits == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    
//     BaseType_t wait_all = (wait_for_all_bits != 0) ? pdTRUE : pdFALSE;
//     BaseType_t clear_on_exit_flag = (clear_on_exit != 0) ? pdTRUE : pdFALSE;
    
//     EventBits_t result = xEventGroupWaitBits(eg, bits_to_wait, clear_on_exit_flag, wait_all, timeout_ticks);
    
//     *actual_bits = result;
    
//     if ((result & bits_to_wait) != 0) {
//         return SPP_OK;
//     } else {
//         return SPP_ERROR;
//     }
// }

// /**
//  * @brief Synchronize tasks using event group
//  */
// retval_t OSAL_EventGroupSync(osal_eventgroup_handle_t event_group,
//                             osal_eventbits_t bits_to_set,
//                             osal_eventbits_t bits_to_wait,
//                             spp_uint32_t timeout_ms,
//                             osal_eventbits_t* achieved_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     if (achieved_bits == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    
//     EventBits_t result = xEventGroupSync(eg, bits_to_set, bits_to_wait, timeout_ticks);
    
//     *achieved_bits = result;
    
//     if ((result & bits_to_wait) != 0) {
//         return SPP_OK;
//     } else {
//         return SPP_ERROR;
//     }
// }

// /**
//  * @brief Clear bits in an event group
//  */
// retval_t OSAL_EventGroupClearBits(osal_eventgroup_handle_t event_group,
//                                  osal_eventbits_t bits_to_clear,
//                                  osal_eventbits_t* previous_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     EventBits_t previous = xEventGroupClearBits(eg, bits_to_clear);
    
//     if (previous_bits != NULL) {
//         *previous_bits = previous;
//     }
    
//     return SPP_OK;
// }

// /**
//  * @brief Get the current bits of an event group
//  */
// retval_t OSAL_EventGroupGetBits(osal_eventgroup_handle_t event_group,
//                                osal_eventbits_t* current_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     if (current_bits == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     *current_bits = xEventGroupGetBits(eg);
    
//     return SPP_OK;
// }

// /**
//  * @brief Get the current bits of an event group from ISR
//  */
// retval_t OSAL_EventGroupGetBitsFromISR(osal_eventgroup_handle_t event_group,
//                                       osal_eventbits_t* current_bits)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     if (current_bits == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     *current_bits = xEventGroupGetBitsFromISR(eg);
    
//     return SPP_OK;
// }

// /**
//  * @brief Delete an event group
//  */
// retval_t OSAL_EventGroupDelete(osal_eventgroup_handle_t event_group)
// {
//     if (event_group == NULL) {
//         return SPP_ERROR_NULL_POINTER;
//     }
    
//     EventGroupHandle_t eg = (EventGroupHandle_t)event_group;
//     vEventGroupDelete(eg);
    
//     return SPP_OK;
// }