/**
 * @file port/task.c
 * @brief OSAL task port for ESP-IDF 
 * 
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "esp_err.h"
#include "osal/task.h"
#include "core/returntypes.h"
#include "core/types.h"
#include <stdbool.h>

/** max tasks handled by this OSAL */
#define SPP_OSAL_MAX_TASKS   7

/** static TCBs one per slot */
static StaticTask_t s_tcbs[SPP_OSAL_MAX_TASKS];
/** static stacks sized in words from a byte budget */
static StackType_t  s_stacks[SPP_OSAL_MAX_TASKS][ SPP_OSAL_STACK_BYTES / sizeof(StackType_t) ];
/** FreeRTOS handles per slot */
static TaskHandle_t s_handles[SPP_OSAL_MAX_TASKS];
/** last wake tick per task for delay until */
static TickType_t   s_last_wake[SPP_OSAL_MAX_TASKS];
/** slot usage flags */
static bool s_used[SPP_OSAL_MAX_TASKS];

/**
 * @brief create a task using a static slot
 * @param task_function entry function pointer
 * @param name short task name
 * @param stack_size requested bytes validated against SPP_OSAL_STACK_BYTES
 * @param p_parameters user parameter passed to the task
 * @param priority native priority value
 * @param task_handle out handle 
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER any required pointer was null
 * @retval SPP_ERROR_INVALID_PARAMETER stack_size was zero or too large
 * @retval SPP_ERROR_NO_MEMORY no free slot available
 * @retval SPP_ERROR FreeRTOS did not create the task
 */
SppRetVal_t SPP_OSAL_TaskCreate(void* task_function, const char* name,
                         spp_uint32_t stack_size, void* p_parameters, spp_uint32_t priority,
                         void** task_handle)
{
    if (task_function == NULL || name == NULL || task_handle == NULL) {
        return SPP_ERROR_NULL_POINTER;
    }
    if (stack_size == 0 || stack_size > SPP_OSAL_STACK_BYTES){
        return SPP_ERROR_INVALID_PARAMETER;
    }

    int slot = -1;
    for (int i = 0; i < SPP_OSAL_MAX_TASKS; i++){
        if (s_used[i] == false){
            s_used[i] = true;
            slot = i;
            break;
        }
    }
    if (slot == -1){
        return SPP_ERROR_NO_MEMORY;
    }

    uint32_t depth_words = (uint32_t)(SPP_OSAL_STACK_BYTES / sizeof(StackType_t));
    TaskHandle_t xHandle = xTaskCreateStatic((TaskFunction_t)task_function, name, depth_words,
                                             p_parameters, (UBaseType_t)priority,
                                             s_stacks[slot], &s_tcbs[slot]);
    if (xHandle == NULL) {
        s_used[slot] = false;
        return SPP_ERROR;
    }

    s_handles[slot]   = xHandle;
    s_last_wake[slot] = xTaskGetTickCount();

    *task_handle = (void*)xHandle;
    return SPP_OK;
}

/**
 * @brief delete a task and free its slot
 * @param task_handle handle to delete
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER handle was null
 */
SppRetVal_t SPP_OSAL_TaskDelete(void* task_handle){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;

    vTaskDelete((TaskHandle_t)task_handle);

    int slot_handle = -1;
    for (int i = 0; i < SPP_OSAL_MAX_TASKS; i++){
        if (s_used[i] && s_handles[i] == (TaskHandle_t)task_handle){
            slot_handle = i;
            break;
        }
    }
    if (slot_handle >= 0) {
        s_used[slot_handle] = false;
        s_handles[slot_handle] = NULL;
        s_last_wake[slot_handle] = 0;
    }
    return SPP_OK;
}

/**
 * @brief suspend a task by handle
 * @param task_handle handle to suspend
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER handle was null
 */
SppRetVal_t SPP_OSAL_TaskSuspend(void* task_handle){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskSuspend((TaskHandle_t)task_handle);
    return SPP_OK;
}

/**
 * @brief resume a previously suspended task
 * @param task_handle handle to resume
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER handle was null
 */
SppRetVal_t SPP_OSAL_TaskResume(void* task_handle){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskResume((TaskHandle_t)task_handle);
    return SPP_OK;
}

/**
 * @brief relative delay for the current task
 * @param delay_ms milliseconds to wait
 * @retval SPP_OK always
 */
SppRetVal_t SPP_OSAL_TaskDelay(spp_uint32_t delay_ms){
    const TickType_t delay_ticks = pdMS_TO_TICKS(delay_ms);
    vTaskDelay(delay_ticks);
    return SPP_OK;
}

/**
 * @brief periodic delay for the current task keeping phase
 * @param delay_ms period in milliseconds
 * @retval SPP_OK always
 * @note relies on a per task last wake stored in this module
 */
SppRetVal_t SPP_OSAL_TaskDelayUntil(spp_uint32_t delay_ms){
    TaskHandle_t handle = xTaskGetCurrentTaskHandle();
    int slot_handle = -1;
    for (int i = 0; i < SPP_OSAL_MAX_TASKS; i++){
        if (s_used[i] && s_handles[i] == handle){
            slot_handle = i;
            break;
        }
    }
    const TickType_t xFrequency = pdMS_TO_TICKS(delay_ms);
    if (s_last_wake[slot_handle] == 0) {
        s_last_wake[slot_handle] = xTaskGetTickCount();
    }
    vTaskDelayUntil(&s_last_wake[slot_handle], xFrequency);
    return SPP_OK;
}

/**
 * @brief set a task priority
 * @param task_handle handle to change
 * @param newpriority native priority value
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER handle was null
 */
SppRetVal_t SPP_OSAL_TaskPrioritySet(void* task_handle, spp_uint32_t newpriority){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskPrioritySet((TaskHandle_t)task_handle, (UBaseType_t)newpriority);
    return SPP_OK;
}

/**
 * @brief get a task priority or current task if handle is null
 * @param task_handle handle or null
 * @return native priority as unsigned integer
 */
spp_uint32_t SPP_OSAL_TaskPriorityGet(void* task_handle){
    TaskHandle_t handle = (task_handle != NULL) ? (TaskHandle_t)task_handle : xTaskGetCurrentTaskHandle();
    UBaseType_t priority = uxTaskPriorityGet(handle);
    return (spp_uint32_t)priority; 
}

/**
 * @brief suspend the scheduler
 * @retval SPP_OK always
 */
SppRetVal_t SPP_OSAL_SuspendAll(void){
    vTaskSuspendAll();
    return SPP_OK;
}

/**
 * @brief resume the scheduler
 * @retval SPP_OK always
 */
SppRetVal_t SPP_OSAL_ResumeAll(void){
    (void)xTaskResumeAll();
    return SPP_OK;
}

/**
 * @brief get the current task handle
 * @return opaque pointer to current FreeRTOS task
 */
void* SPP_OSAL_TaskGetCurrent(void){
    return (void*)xTaskGetCurrentTaskHandle();
}

/**
 * @brief get a task state or current task if handle is null
 * @param task_handle handle or null
 * @return state mapped to your SppTaskState
 */
SppTaskState SPP_OSAL_TaskGetState(void* task_handle){
    TaskHandle_t handle = (task_handle != NULL) ? (TaskHandle_t)task_handle : xTaskGetCurrentTaskHandle();
    eTaskState state = eTaskGetState(handle);
    switch (state){
        case eReady:     return SPP_OSAL_TASK_READY;
        case eRunning:   return SPP_OSAL_TASK_RUNNING;
        case eBlocked:   return SPP_OSAL_TASK_BLOCKED;
        case eSuspended: return SPP_OSAL_TASK_SUSPENDED;
        case eDeleted:   return SPP_OSAL_TASK_DELETED;
        default:         return SPP_OSAL_TASK_DELETED;
    }
}

/**
 * @brief yield the CPU voluntarily
 * @retval SPP_OK always
 */
SppRetVal_t SPP_OSAL_TaskYield(void){
    taskYIELD();
    return SPP_OK;
}

/**
 * @brief register idle hook called in the idle task
 * @param idle_function function pointer returning bool must not block
 * @retval SPP_OK on success
 * @retval SPP_ERROR_NULL_POINTER idle_function was null
 * @retval SPP_ERROR registration failed
 * @note uses esp_register_freertos_idle_hook which returns a boolean
 */
SppRetVal_t SPP_OSAL_IdleHookRegister(bool (*idle_function)(void)){
    if (idle_function == NULL) return SPP_ERROR_NULL_POINTER;
    return esp_register_freertos_idle_hook(idle_function) ? SPP_OK : SPP_ERROR;
}
