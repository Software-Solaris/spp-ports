/**
 * @file gpio.c
 * @brief ESP32 GPIO HAL implementation for the SPP framework.
 *
 * Provides GPIO interrupt configuration and ISR registration, bridging
 * ESP-IDF gpio driver calls to the SPP OSAL event group mechanism.
 */

/* ============================================================================
 * Includes
 * ========================================================================= */

#include "spp/hal/gpio/gpio.h"
#include "spp/core/returntypes.h"
#include "spp/core/types.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#include "spp/osal/eventgroups.h"

/* ============================================================================
 * Private Functions
 * ========================================================================= */

/**
 * @brief Internal GPIO ISR handler.
 *
 * Sets the configured event group bits from ISR context and yields to a
 * higher-priority task if one was unblocked.
 *
 * @param[in] p_arg Pointer to the spp_gpio_isr_ctx_t context structure.
 */
static void gpio_internal_isr(void *p_arg)
{
    spp_gpio_isr_ctx_t *p_ctx = (spp_gpio_isr_ctx_t *)p_arg;

    spp_uint8_t hpw = 0;
    OSAL_EventGroupSetBitsFromISR(p_ctx->p_event_group, p_ctx->bits, NULL, &hpw);

    if (hpw != 0)
    {
        portYIELD_FROM_ISR();
    }
}

/* ============================================================================
 * Public Functions
 * ========================================================================= */

/**
 * @brief Configure a GPIO pin as an interrupt input.
 *
 * Sets the pin as input with the specified interrupt type and optional
 * pull-up or pull-down resistor.
 *
 * @param[in] pin       GPIO pin number.
 * @param[in] intr_type Interrupt trigger type (ESP-IDF gpio_int_type_t value).
 * @param[in] pull      Pull resistor config: 0 = none, 1 = pull-up, 2 = pull-down.
 * @return SPP_OK on success.
 */
retval_t SPP_HAL_GPIO_ConfigInterrupt(spp_uint32_t pin, spp_uint32_t intr_type, spp_uint32_t pull)
{
    gpio_config_t ioCfg;

    ioCfg.pin_bit_mask = (1ULL << pin);
    ioCfg.mode = GPIO_MODE_INPUT;
    ioCfg.intr_type = (gpio_int_type_t)intr_type;

    ioCfg.pull_up_en = GPIO_PULLUP_DISABLE;
    ioCfg.pull_down_en = GPIO_PULLDOWN_DISABLE;

    if (pull == 1)
    {
        ioCfg.pull_up_en = GPIO_PULLUP_ENABLE;
    }
    else if (pull == 2)
    {
        ioCfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
    }

    gpio_config(&ioCfg);
    return SPP_OK;
}

/**
 * @brief Register an ISR handler for a GPIO pin.
 *
 * Installs the ESP-IDF GPIO ISR service on the first call, then adds the
 * internal ISR handler for the specified pin. The ISR context is passed
 * through to the handler and should point to a spp_gpio_isr_ctx_t.
 *
 * @param[in] pin         GPIO pin number.
 * @param[in] p_isrContext Pointer to the spp_gpio_isr_ctx_t for this pin.
 * @return SPP_OK on success.
 */
retval_t SPP_HAL_GPIO_RegisterISR(spp_uint32_t pin, void *p_isrContext)
{
    static spp_uint8_t s_isrServiceInstalled = 0;

    if (s_isrServiceInstalled == 0)
    {
        gpio_install_isr_service(0);
        s_isrServiceInstalled = 1;
    }

    gpio_isr_handler_add((gpio_num_t)pin, gpio_internal_isr, p_isrContext);
    return SPP_OK;
}
