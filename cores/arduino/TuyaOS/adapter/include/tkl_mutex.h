/**
* @file tkl_mutex.h
* @brief Common process - adapter the mutex api provide by OS
* @version 0.1
* @date 2020-11-09
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_MUTEX_H__
#define __TKL_MUTEX_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef VOID_T* TKL_MUTEX_HANDLE;

/**
* @brief Create mutex
*
* @param[out] pMutexHandle: mutex handle
*
* @note This API is used to create and init mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_create_init(TKL_MUTEX_HANDLE *pMutexHandle);

/**
* @brief Lock mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to lock mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_lock(CONST TKL_MUTEX_HANDLE mutexHandle);


/**
* @brief Unlock mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to unlock mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_unlock(CONST TKL_MUTEX_HANDLE mutexHandle);

/**
* @brief Release mutex
*
* @param[in] mutexHandle: mutex handle
*
* @note This API is used to release mutex.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_mutex_release(CONST TKL_MUTEX_HANDLE mutexHandle);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
