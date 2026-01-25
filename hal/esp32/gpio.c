#include "hal/gpio/gpio.h"
#include "core/returntypes.h"
#include "core/types.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "osal/eventgroups.h"

static void gpio_internal_isr(void* arg)
{
    spp_gpio_isr_ctx_t* ctx = (spp_gpio_isr_ctx_t*)arg;

    spp_uint8_t hpw = 0;
    OSAL_EventGroupSetBitsFromISR(ctx->event_group, ctx->bits, NULL, &hpw);

    if (hpw != 0) {
        portYIELD_FROM_ISR();
    }
}

retval_t SPP_HAL_GPIO_ConfigInterrupt(spp_uint32_t pin, spp_uint32_t intr_type, spp_uint32_t pull)
{
    gpio_config_t io_conf;

    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.intr_type    = (gpio_int_type_t)intr_type;

    io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    if (pull == 1) {
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    } else if (pull == 2) {
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }

    gpio_config(&io_conf);
    return SPP_OK;
}

retval_t SPP_HAL_GPIO_RegisterISR(spp_uint32_t pin, void* isr_context)
{
    static spp_uint8_t s_isr_service_installed = 0;

    if (s_isr_service_installed == 0) {
        gpio_install_isr_service(0); 
        s_isr_service_installed = 1;
    }

    gpio_isr_handler_add((gpio_num_t)pin, gpio_internal_isr, isr_context);
    return SPP_OK;
}