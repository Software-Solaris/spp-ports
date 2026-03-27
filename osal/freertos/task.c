/**
 * @file task.c
 * @brief FreeRTOS OSAL task implementation for the SPP framework.
 *
 * Provides static task creation from a pre-allocated pool, task deletion,
 * and millisecond-based delay using FreeRTOS primitives.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "spp/osal/task.h"
#include "spp/core/types.h"
#include "spp/core/macros.h"

/* ============================================================================
 * Private Constants
 * ========================================================================= */

/** @brief Maximum number of statically allocated tasks. */
#define K_MAX_TASKS 50

/** @brief Stack depth (in StackType_t words) for each task. */
#define K_MAX_STACK 4096

/* ============================================================================
 * Private Types
 * ========================================================================= */

/**
 * @brief Static storage for a single FreeRTOS task (stack + TCB buffer).
 */
typedef struct
{
    StackType_t stack[K_MAX_STACK];
    StaticTask_t buffer;
} TaskStorage_t;

/* ============================================================================
 * Private Variables
 * ========================================================================= */

/** @brief Pool of pre-allocated task storage slots. */
static TaskStorage_t s_taskPool[K_MAX_TASKS];

/** @brief Number of task storage slots currently allocated. */
static uint32_t s_taskCount = 0;

/* ============================================================================
 * Public Functions
 * ========================================================================= */

/**
 * @brief Allocate a task storage slot from the static pool.
 *
 * Each call returns the next available TaskStorage_t from s_taskPool.
 *
 * @return Pointer to the allocated TaskStorage_t, or NULL if the pool is
 *         exhausted.
 */
void *SPP_OSAL_GetTaskStorage()
{
    if (s_taskCount >= K_MAX_TASKS)
    {
        return NULL;
    }
    TaskStorage_t *p_taskStorage = &s_taskPool[s_taskCount];
    s_taskCount += 1;
    return p_taskStorage;
}

/**
 * @brief Create a new FreeRTOS task using static allocation.
 *
 * @param[in] p_function   Task entry function pointer.
 * @param[in] task_name    Human-readable task name string.
 * @param[in] stack_depth  Requested stack depth (unused; actual depth comes
 *                         from the storage pool).
 * @param[in] p_custom_data Opaque pointer passed to the task function.
 * @param[in] priority     FreeRTOS task priority.
 * @param[in] p_storage    Pointer to a TaskStorage_t obtained from
 *                         SPP_OSAL_GetTaskStorage().
 * @return Task handle as void pointer, or NULL on failure.
 */
void *SPP_OSAL_TaskCreate(void *p_function, const char *const task_name, const uint32_t stack_depth,
                          void *const p_custom_data, spp_uint32_t priority, void *p_storage)
{
    if (p_function == NULL || task_name == NULL)
    {
        return NULL;
    }

    TaskStorage_t *p_taskStorage = (TaskStorage_t *)p_storage;

    StackType_t *p_stack = p_taskStorage->stack;
    StaticTask_t *p_taskBuffer = &p_taskStorage->buffer;

    UBaseType_t realStackDepth = sizeof(p_taskStorage->stack) / sizeof(StackType_t);

    TaskHandle_t p_task =
        xTaskCreateStatic((TaskFunction_t)p_function, task_name, realStackDepth, p_custom_data,
                          (UBaseType_t)priority, p_stack, p_taskBuffer);
    if (p_task == NULL)
    {
        return NULL;
    }

    void *p_taskHandle = (void *)p_task;
    return p_taskHandle;
}

/**
 * @brief Delete a FreeRTOS task.
 *
 * If p_task is NULL or contains a NULL handle, the calling task is deleted.
 *
 * @param[in] p_task Pointer to the task handle, or NULL to delete the
 *                   calling task.
 * @return SPP_OK (note: if the calling task is deleted, this does not return).
 */
retval_t SPP_OSAL_TaskDelete(void *p_task)
{
    if (p_task == NULL)
    {
        /* Delete current task */
        vTaskDelete(NULL);
        return SPP_OK;
    }

    TaskHandle_t taskHandle = *(TaskHandle_t *)p_task;

    /* If caller passed a NULL handle inside the pointer, delete current task */
    if (taskHandle == NULL)
    {
        vTaskDelete(NULL);
        return SPP_OK;
    }

    vTaskDelete(taskHandle);
    return SPP_OK;
}

/**
 * @brief Delay the calling task for a specified number of milliseconds.
 *
 * @param[in] blocktime_ms Delay duration in milliseconds.
 */
void SPP_OSAL_TaskDelay(spp_uint32_t blocktime_ms)
{
    vTaskDelay(pdMS_TO_TICKS(blocktime_ms));
}
