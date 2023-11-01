#include "tkl_pinmux.h"

#include "BkDriverGpio.h"
#include "gpio_pub.h"

/**
 * @brief tuya io pinmux func
 *
 * @param[in] pin: pin number
 * @param[in] pin_func: pin function
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief tuya io pin to port,
 * @brief historical legacy, can not use in pin muxtiplex mode
 * @param[in] pin: pin number
 * @param[in] pin: pin type
 * @return Pin Function : Port and Channel,err < 0.
 * @return        16        8       8
 * @return[out] | rsv   |  port | channel |
 */
INT32_T tkl_io_pin_to_func(UINT32_T pin, TUYA_PIN_TYPE_E pin_type)
{
    INT32_T port_channel = OPRT_NOT_SUPPORTED;
    
    switch (pin_type) {
        case TUYA_IO_TYPE_PWM:                  // all pwm channels belong to one port
            if (TUYA_IO_PIN_6 == pin) {
                port_channel = 0;
            } else if (TUYA_IO_PIN_7 == pin) {
                port_channel = 1;
            } else if (TUYA_IO_PIN_8 == pin) {
                port_channel = 2;
            } else if (TUYA_IO_PIN_9 == pin) {
                port_channel = 3;
            } else if (TUYA_IO_PIN_24 == pin) {
                port_channel = 4;
            } else if (TUYA_IO_PIN_26 == pin) {
                port_channel = 5;
            }
            break;
        case TUYA_IO_TYPE_ADC:
            if (TUYA_IO_PIN_26 == pin) {
                port_channel = 0;
            } else if (TUYA_IO_PIN_24 == pin) {
                port_channel = 1;
            } else if (TUYA_IO_PIN_23 == pin) {
                port_channel = 2;
            } else if (TUYA_IO_PIN_28 == pin) {
                port_channel = 3;
            } else if (TUYA_IO_PIN_22 == pin) {
                port_channel = 4;
            } else if (TUYA_IO_PIN_21 == pin) {
                port_channel = 5;
            }
            break;
        case TUYA_IO_TYPE_DAC:
            break;
        case TUYA_IO_TYPE_UART:
            break;
        case TUYA_IO_TYPE_SPI:
            if ((pin >= TUYA_IO_PIN_11) || (pin <= TUYA_IO_PIN_14)) {
                port_channel = 0;
            }
            break;
        case TUYA_IO_TYPE_I2C:
            if ((TUYA_IO_PIN_19 == pin) || (TUYA_IO_PIN_20 == pin)) {
                port_channel = 0;
            } else if ((TUYA_IO_PIN_28 == pin) || (TUYA_IO_PIN_29 == pin)) {
                port_channel = 1;
            }
            break;
        case TUYA_IO_TYPE_I2S:
            break;
        case TUYA_IO_TYPE_GPIO:
            break;
        default:
            break;
    }

    /*
    need use TUYA_IO_GET_PORT_ID to get port
    nedd use TUYA_IO_GET_CHANNEL_ID to get channel
    */
    return port_channel;
}


