#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "osal/task.h"
#include "core/types.h"
#include "core/macros.h"

#define MAX_TASKS   50   
#define MAX_STACK   4096

typedef struct {
    StackType_t stack[MAX_STACK];
    StaticTask_t buffer;
} TaskStorage_t;

static TaskStorage_t task_pool[MAX_TASKS];
static uint32_t task_count = 0;


void * SPP_OSAL_GetTaskStorage(){

    if (task_count >= MAX_TASKS) {
        return NULL;
    }
    TaskStorage_t* p_task_storage = &task_pool[task_count];
    task_count += 1;
    return p_task_storage;
}


void* SPP_OSAL_TaskCreate(void *p_function, const char *const task_name, 
                            const uint32_t stack_depth,void *const p_custom_data,
                            spp_uint32_t priority, void * p_storage)
{
    if (p_function == NULL || task_name == NULL){
        return NULL;
    }

    TaskStorage_t *p_task_storage = (TaskStorage_t*)p_storage;

    StackType_t *xStack = p_task_storage->stack;
    StaticTask_t *xTaskBuffer = &p_task_storage->buffer;

    UBaseType_t real_stack_depth = sizeof(p_task_storage->stack) / sizeof(StackType_t);

    TaskHandle_t p_task = xTaskCreateStatic((TaskFunction_t) p_function, 
                                            task_name, 
                                            real_stack_depth, 
                                            p_custom_data, 
                                            (UBaseType_t) priority, 
                                            xStack, 
                                            xTaskBuffer);
    if (p_task == NULL){
        return NULL;
    }

    void* p_task_handle = (void*)p_task;
    return p_task_handle;
}

retval_t SPP_OSAL_TaskDelete(void *p_task){
    TaskHandle_t *p_task_delete = (TaskHandle_t*)p_task;
    vTaskDelete(*p_task_delete);
    return SPP_OK;
}


void SPP_OSAL_TaskDelay(spp_uint32_t blocktime_ms)
{
    vTaskDelay(pdMS_TO_TICKS(blocktime_ms));
}

