#include "tkl_uart.h"

#include "drv_model_pub.h"
#include "uart_pub.h"
#include "BkDriverUart.h"

/**
 * @brief uart init
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    bk_uart_t port;
    bk_uart_config_t bkcfg;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_1;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_2;
    } else {
        return OPRT_INVALID_PARM;
    }

    //! data bits
    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        bkcfg.data_width = DATA_WIDTH_5BIT;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        bkcfg.data_width = DATA_WIDTH_6BIT;
    } else if (TUYA_UART_DATA_LEN_7BIT ==  cfg->databits) {
        bkcfg.data_width = DATA_WIDTH_7BIT;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        bkcfg.data_width = DATA_WIDTH_8BIT;
    } else {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    //! stop bits
    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        bkcfg.stop_bits = BK_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        bkcfg.stop_bits = BK_STOP_BITS_2;
    } else {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    //!  parity bits
    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        bkcfg.parity = BK_PARITY_NO;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        bkcfg.parity = BK_PARITY_EVEN;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        bkcfg.parity = BK_PARITY_ODD;
    } else {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    //! baudrate
    bkcfg.baud_rate    = cfg->baudrate;
    //! flow control
    bkcfg.flow_control = 0;
    bkcfg.flags        = 0;

    bk_uart_initialize(port, &bkcfg, NULL);

    return OPRT_OK;
}

/**
 * @brief uart deinit
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    bk_uart_t port;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_1;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_2;
    } else {
        return OPRT_INVALID_PARM;
    }

    bk_uart_diable_rx(port);
    bk_uart_finalize(port);
    return OPRT_OK;
}

/**
 * @brief uart write data
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */
extern void bk_send_byte(UINT8 uport, UINT8 data);
INT_T tkl_uart_write(TUYA_UART_NUM_E port_id, VOID_T *buff, UINT16_T len)
{
    extern void bk_send_byte(UINT8 uport, UINT8 data);
    bk_uart_t port;
    int i;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_1;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_2;
    } else {
        return OPRT_INVALID_PARM;
    }

    for ( i = 0; i < len; i++) {
        bk_send_byte( port, *(UINT8*)(buff+i));
    }
    
    return i;
}

/**
 * @brief uart read data
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 * 
 * @return return >= 0: number of data read; return < 0: read errror
 */
INT_T tkl_uart_read(TUYA_UART_NUM_E port_id, VOID_T *buff, UINT16_T len)
{
    bk_uart_t port;
    int result;
    int i;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_1;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_2;
    } else {
        return OPRT_INVALID_PARM;
    }

    for ( i = 0; i < len; i++) {
        result = uart_read_byte(port);
        if (-1 != result) {
            *(uint8_t*)(buff+i)  = (uint8_t) result;
        }
        else {
            break;
        }
    }
    return i;
}

/**
 * @brief set uart transmit interrupt status
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe tx int, FALSE-disable tx int
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    return OPRT_OK;
}

/**
 * @brief set uart receive flowcontrol
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe rx flowcontrol, FALSE-disable rx flowcontrol
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_rx_flowctrl(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    return OPRT_OK;
}

void uart_dev_irq_handler(int uport, void *param)
{
    TUYA_UART_IRQ_CB uart_irq_cb = (TUYA_UART_IRQ_CB)param;

    uart_irq_cb((UINT_T)uport);
}

/**
 * @brief enable uart rx interrupt and regist interrupt callback
 * 
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
VOID_T tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    bk_uart_t port;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_1;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = BK_UART_2;
    } else {
        return ;
    }

    bk_uart_set_rx_callback(port, uart_dev_irq_handler, rx_cb);
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *  
 * @param[in] port_id: uart port id, 
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
VOID_T tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    
}
