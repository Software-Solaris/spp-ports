#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "osal/task.h"
#include "core/types.h"
#include "core/macros.h"


void* SPP_OSAL_TaskCreate(void *p_function, const char *const task_name, 
                            const uint32_t stack_depth,void *const p_custom_data,
                            spp_uint32_t priority, uint64_t stack_size)
{
    if (p_function == NULL || task_name == NULL){
        return NULL;
    }

    StackType_t xStack[stack_size];
    StaticTask_t xTaskBuffer;

    TaskHandle_t p_task = xTaskCreateStatic((TaskFunction_t) p_function, 
                                            task_name, 
                                            stack_depth, 
                                            p_custom_data, 
                                            (UBaseType_t) priority, 
                                            &xStack, 
                                            &xTaskBuffer);
    if (p_task == NULL){
        return NULL;
    }

    void* p_task_handle = (void*)p_task;
    return p_task_handle;
}