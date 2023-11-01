/**
 * @file uni_thread.h
 * @brief tuya thread module
 * @version 1.0
 * @date 2019-10-30
 * 
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 * 
 */


#ifndef __TAL_THREAD_H__
#define __TAL_THREAD_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef VOID_T* THREAD_HANDLE;

/**
 * @brief max length of thread name
 * 
 */
#define TAL_THREAD_MAX_NAME_LEN     16

/**
 * @brief thread process function
 * 
 */
typedef VOID (*THREAD_FUNC_CB)(PVOID_T args);
/**
 * @brief thread enter function
 * 
 */
typedef VOID(*THREAD_ENTER_CB)(VOID); 

/**
 * @brief thread exut function
 * 
 */
typedef VOID(*THREAD_EXIT_CB)(VOID); // thread extract
/**
 * @brief thread running status
 * 
 */
typedef enum {
    THREAD_STATE_EMPTY = 0,
    THREAD_STATE_RUNNING,
    THREAD_STATE_STOP,
    THREAD_STATE_DELETE,
} THREAD_STATE_E;

/**
 * @brief thread priority
 * 
 */
typedef enum {
    THREAD_PRIO_0 = 5,
    THREAD_PRIO_1 = 4,
    THREAD_PRIO_2 = 3,
    THREAD_PRIO_3 = 2,
    THREAD_PRIO_4 = 1,
    THREAD_PRIO_5 = 0,
    THREAD_PRIO_6 = 0,
} THREAD_PRIO_E;
/**
 * @brief thread parameters
 * 
 */
typedef struct {
    UINT_T          stackDepth;     // stack size 
    UINT8_T         priority;       // thread priority
    CHAR_T         *thrdname;       // thread name
} THREAD_CFG_T;

/**
 * @brief create and start a tuya sdk thread
 * 
 * @param[in] enter: the function called before the thread process called.can be null
 * @param[in] exit: the function called after the thread process called.can be null
 * @param[in] func: the main thread process function
 * @param[in] func_args: the args of the pThrdFunc.can be null
 * @param[in] cfg: the param of creating a thread
 * @param[out] handle: the tuya sdk thread context
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h    
 */
OPERATE_RET tal_thread_create_and_start(THREAD_HANDLE          *handle,
                                        CONST THREAD_ENTER_CB   enter,
                                        CONST THREAD_EXIT_CB    exit,
                                        CONST THREAD_FUNC_CB    func,
                                        CONST PVOID_T           func_args,
                                        CONST THREAD_CFG_T     *cfg);
/**
 * @brief stop and free a tuya sdk thread
 * 
 * @param[in] handle: the input thread context
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h    
 */
OPERATE_RET tal_thread_delete(CONST THREAD_HANDLE handle);

/**
 * @brief check the function caller is in the input thread context
 * 
 * @param[in] handle: the input thread context
 * @param[in] bl: run in self space
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h    
 */
OPERATE_RET tal_thread_is_self(CONST THREAD_HANDLE handle, BOOL_T *bl);

/**
 * @brief get the thread context running status
 * 
 * @param[in] thrdHandle: the input thread context
 * @return the thread status
 */
THREAD_STATE_E tal_thread_get_state(CONST THREAD_HANDLE handle);

/**
 * @brief diagnose the thread(dump task stack, etc.)
 * 
 * @param[in] thrdHandle: the input thread context
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h    
 */
OPERATE_RET tal_thread_diagnose(CONST THREAD_HANDLE handle);

VOID tal_thread_dump_stack(VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

