/**
 * @file tkl_timer.h
 * @brief Common process - adapter the hardware timer api
 * @version 0.1
 * @date 2021-08-06
 *
 * @copyright Copyright 2018-2021 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_TIMER_H__
#define __TKL_TIMER_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief timer init
 * 
 * @param[in] timer_id timer id
 * @param[in] cfg timer configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_init(TUYA_TIMER_NUM_E timer_id, TUYA_TIMER_BASE_CFG_E *cfg);

/**
 * @brief timer start
 * 
 * @param[in] timer_id timer id
 * @param[in] us when to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_start(TUYA_TIMER_NUM_E timer_id, UINT_T us);

/**
 * @brief timer stop
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_stop(TUYA_TIMER_NUM_E timer_id);

/**
 * @brief timer deinit
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_deinit(TUYA_TIMER_NUM_E timer_id);

/**
 * @brief timer get
 * 
 * @param[in] timer_id timer id
 * @param[out] ms timer interval
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get(TUYA_TIMER_NUM_E timer_id, UINT_T *us);



#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_TIMER_H__

