/**
 * @file tkl_queue.h
 * @brief  Common process - Initialization
 * @version 0.1
 * @date 2021-01-04
 *
 * @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TKL_QUEUE_H__
#define __TKL_QUEUE_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TKL_QUEUE_WAIT_FROEVER 0xFFFFFFFF
typedef VOID_T* TKL_QUEUE_HANDLE;



/**
 * @brief Create message queue
 *
 * @param[in] msgsize message size
 * @param[in] msgcount message number
 * @param[out] queue the queue handle created
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, INT_T msgsize, INT_T msgcount);

/**
 * @brief post a message to the message queue
 *
 * @param[in] queue the handle of the queue
 * @param[in] data the data of the message
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_post(CONST TKL_QUEUE_HANDLE queue, VOID_T *data, UINT_T timeout);

/**
 * @brief fetch message from the message queue
 *
 * @param[in] queue the message queue handle
 * @param[inout] msg the message fetch form the message queue
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_fetch(CONST TKL_QUEUE_HANDLE queue, VOID_T *msg, UINT_T timeout);

/**
 * @brief free the message queue
 *
 * @param[in] queue the message queue handle
 *
 * @return VOID_T
 */
VOID_T tkl_queue_free(CONST TKL_QUEUE_HANDLE queue);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
