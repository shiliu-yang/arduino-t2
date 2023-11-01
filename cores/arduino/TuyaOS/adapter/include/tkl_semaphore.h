/**
* @file tkl_semaphore.h
* @brief Common process - adapter the semaphore api provide by OS
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_SEMAPHORE_H__
#define __TKL_SEMAPHORE_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef VOID_T* TKL_SEM_HANDLE;
#define TKL_SEM_WAIT_FOREVER 0xFFFFffff

/**
* @brief Create semaphore
*
* @param[out] handle: semaphore handle
* @param[in] sem_cnt: semaphore init count
* @param[in] sem_max: semaphore max count
*
* @note This API is used for creating semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_create_init(TKL_SEM_HANDLE *handle, UINT_T sem_cnt, UINT_T sem_max);

/**
* @brief Wait semaphore
*
* @param[in] handle: semaphore handle
* @param[in] timeout: wait timeout, SEM_WAIT_FOREVER means wait until get semaphore
*
* @note This API is used for waiting semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_wait(CONST TKL_SEM_HANDLE handle, UINT_T timeout);

/**
* @brief Post semaphore
*
* @param[in] handle: semaphore handle
*
* @note This API is used for posting semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_post(CONST TKL_SEM_HANDLE handle);

/**
* @brief Release semaphore
*
* @param[in] handle: semaphore handle
*
* @note This API is used for releasing semaphore.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_semaphore_release(CONST TKL_SEM_HANDLE handle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

