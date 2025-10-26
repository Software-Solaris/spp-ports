#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "osal/task.h"
#include "core/returntypes.h"
#include "core/types.h"


static StaticTask_t s_task_tcb;
static StackType_t  s_task_stack[OSAL_STACK_MAX_BYTES];

SppRetVal_t SPP_OSAL_TaskCreate(void* task_function, const char* name,
                         spp_uint32_t stack_size, void* p_parameters, SppPriority_t priority,
                         void* p_task_handle)
{   
    if (task_function == NULL || name == NULL || task_handle == NULL) {
        return SPP_ERROR_NULL_POINTER;
    }   
    if (stack_size_bytes == 0 || stack_size_bytes > SPP_MAX_STACK_BYTES){
        return SPP_ERROR_INVALID_PARAMETER;
    }   

    TaskHandle_t xHandle= (TaskHandle_t)p_task_handle;
    xHandle = xTaskCreateStatic(task_function, name, stack_size, p_parameters, priority,
          s_task_stack, &s_task_tcb);
    if (xHandle == NULL) {
        return SPP_ERROR;
    }

    *task_handle = (void*)xHandle;
    return SPP_OK;
}

