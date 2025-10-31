#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "osal/task.h"
#include "core/types.h"
#include "core/macros.h"

void* SPP_OSAL_TaskCreate(void *p_function, const char *const task_name, 
                            const uint32_t stack_depth,void *const p_custom_data,
                            spp_uint32_t priority, void* p_stack_buffer,
                            void* p_task_buffer )
{
    if (p_function == NULL || task_name == NULL || p_stack_buffer == NULL || p_task_buffer == NULL){
        return NULL;
    }
    TaskHandle_t p_task = xTaskCreateStatic((TaskFunction_t) p_function, 
                                                    task_name, 
                                                    stack_depth, 
                                                    p_custom_data, 
                                                    (UBaseType_t) priority, 
                                                    (StackType_t *) p_stack_buffer, 
                                                    (StaticTask_t *) p_task_buffer);
    if (p_task == NULL){
        return NULL;
    }

    void* p_task_handle = (void*)p_task;
    return p_task_handle;
}