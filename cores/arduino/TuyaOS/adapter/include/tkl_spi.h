/**
* @file tkl_spi.h
* @brief Common process - adapter the spi api
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_SPI_H__
#define __TKL_SPI_H__

#include "tuya_cloud_types.h"
#include "tkl_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief spi init
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_BASE_CFG_T *cfg);

/**
 * @brief spi deinit
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port);

/**
 * Spi send
 *
 * @param[in]  port      the spi device
 * @param[in]  data     spi send data
 * @param[in]  size     spi send data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size);

/**
 * spi_recv
 *
 * @param[in]   port      the spi device
 * @param[out]  data     spi recv data
 * @param[in]   size     spi recv data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size);

/**
 * @brief spi transfer
 * 
 * @param[in] port: spi port
 * @param[in] send_buf: spi send buf
 * @param[out] send_buf:spi recv buf
 * @param[in] length: spi msg length
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, VOID_T* send_buf, VOID_T* receive_buf, UINT32_T length);

/**
 * @brief adort spi transfer,or spi send, or spi recv
 * 
 * @param[in] port: spi port
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port);

/**
 * @brief get spi status.
 * 
 * @param[in] port: spi port
 * @param[out]  TUYA_SPI_STATUS_T,please refer to tuya_cloud_types.h
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status); 

/**
 * @brief spi irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: spi port, id index starts at 0
 * @param[in] cb:  spi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_IRQ_CB *cb);

/**
 * @brief spi irq enable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port);

/**
 * @brief spi irq disable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


