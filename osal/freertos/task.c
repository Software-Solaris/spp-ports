#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "esp_err.h"
#include "osal/task.h"
#include "core/returntypes.h"
#include "core/types.h"
#include <stdbool.h>

#define SPP_OSAL_MAX_TASKS   7

static StaticTask_t s_tcbs[SPP_OSAL_MAX_TASKS];
static StackType_t  s_stacks[SPP_OSAL_MAX_TASKS][ SPP_OSAL_STACK_BYTES / sizeof(StackType_t) ];
static TaskHandle_t s_handles[SPP_OSAL_MAX_TASKS];
static TickType_t   s_last_wake[SPP_OSAL_MAX_TASKS];
static bool         s_used[SPP_OSAL_MAX_TASKS];


SppRetVal_t SPP_OSAL_TaskCreate(void* task_function, const char* name,
                         spp_uint32_t stack_size, void* p_parameters, SppPriority_t priority,
                         SppTaskHandle_t* task_handle)
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

    *task_handle = (SppTaskHandle_t)xHandle;
    return SPP_OK;
}

SppRetVal_t SPP_OSAL_TaskDelete(SppTaskHandle_t task_handle){
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

SppRetVal_t SPP_OSAL_TaskSuspend(SppTaskHandle_t task_handle){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskSuspend((TaskHandle_t)task_handle);
    return SPP_OK;
}

SppRetVal_t SPP_OSAL_TaskResume(SppTaskHandle_t task_handle){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskResume((TaskHandle_t)task_handle);
    return SPP_OK;
}

SppRetVal_t SPP_OSAL_TaskDelay(spp_uint32_t delay_ms){
    const TickType_t delay_ticks = pdMS_TO_TICKS(delay_ms);
    vTaskDelay(delay_ticks);
    return SPP_OK;
}

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

SppRetVal_t SPP_OSAL_TaskPrioritySet(SppTaskHandle_t task_handle, SppPriority_t newpriority){
    if (task_handle == NULL) return SPP_ERROR_NULL_POINTER;
    vTaskPrioritySet((TaskHandle_t)task_handle, (UBaseType_t)newpriority);
    return SPP_OK;
}

SppPriority_t SPP_OSAL_TaskPriorityGet(SppTaskHandle_t task_handle){
    TaskHandle_t handle = (task_handle != NULL) ? (TaskHandle_t)task_handle : xTaskGetCurrentTaskHandle();
    UBaseType_t priority = uxTaskPriorityGet(handle);
    switch (priority){
        case 0: return SPP_OSAL_PRIORITY_IDLE;
        case 1: return SPP_OSAL_PRIORITY_LOW;
        case 2: return SPP_OSAL_PRIORITY_NORMAL;
        case 3: return SPP_OSAL_PRIORITY_HIGH;
        default: return SPP_OSAL_PRIORITY_CRITICAL;
    }
}

SppRetVal_t SPP_OSAL_SuspendAll(void){
    vTaskSuspendAll();
    return SPP_OK;
}

SppRetVal_t SPP_OSAL_ResumeAll(void){
    (void)xTaskResumeAll();
    return SPP_OK;
}

void* SPP_OSAL_TaskGetCurrent(void){
    return (void*)xTaskGetCurrentTaskHandle();
}

SppTaskState SPP_OSAL_TaskGetState(SppTaskHandle_t task_handle){
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

SppRetVal_t SPP_OSAL_TaskYield(void){
    taskYIELD();
    return SPP_OK;
}

SppRetVal_t SPP_OSAL_IdleHookRegister(bool (*idle_function)(void)){
    if (idle_function!=NULL) return SPP_ERROR_NULL_POINTER;
    return (esp_register_freertos_idle_hook(idle_function) == ESP_OK) ? SPP_OK : SPP_ERROR;
}