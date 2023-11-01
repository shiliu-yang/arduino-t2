/**
 * @file tkl_system.c
 * @brief the default weak implements of tuya os system api, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_system.h"

#include "start_type_pub.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtos_pub.h"
#include "BkDriverRng.h"
#include "wlan_ui_pub.h"

/**
* @brief Get system ticket count
*
* @param VOID
*
* @note This API is used to get system ticket count.
*
* @return system ticket count
*/
SYS_TICK_T tkl_system_get_tick_count(VOID_T)
{
    return (SYS_TICK_T)xTaskGetTickCount();
}

/**
* @brief Get system millisecond
*
* @param none
*
* @return system millisecond
*/
SYS_TIME_T tkl_system_get_millisecond(VOID_T)
{
    return (SYS_TIME_T)(tkl_system_get_tick_count() * portTICK_RATE_MS);
}

/**
* @brief System sleep
*
* @param[in] msTime: time in MS
*
* @note This API is used for system sleep.
*
* @return VOID
*/
VOID_T tkl_system_sleep(CONST UINT_T num_ms)
{
    UINT_T ticks = num_ms / portTICK_RATE_MS;

    if (ticks == 0) {
        ticks = 1;
    }

    vTaskDelay(ticks);
}


/**
* @brief System reset
*
* @param VOID
*
* @note This API is used for system reset.
*
* @return VOID
*/
VOID_T tkl_system_reset(VOID_T)
{
    bk_reboot();
	return;
}

/**
* @brief Get free heap size
*
* @param VOID
*
* @note This API is used for getting free heap size.
*
* @return size of free heap
*/
INT_T tkl_system_get_free_heap_size(VOID_T)
{
    return (INT_T)xPortGetFreeHeapSize();
}

/**
* @brief Get system reset reason
*
* @param VOID
*
* @note This API is used for getting system reset reason.
*
* @return reset reason of system
*/
TUYA_RESET_REASON_E tkl_system_get_reset_reason(CHAR_T** describe)
{
    unsigned char value = bk_misc_get_start_type() & 0xFF;
    TUYA_RESET_REASON_E bk_value;

    switch (value) {
        case RESET_SOURCE_POWERON:
            bk_value = TUYA_RESET_REASON_POWERON;
            break;

        case RESET_SOURCE_REBOOT:
            bk_value = TUYA_RESET_REASON_SOFTWARE;
            break;

        case RESET_SOURCE_WATCHDOG:
            bk_value = TUYA_RESET_REASON_HW_WDOG;
            break;
        
        case RESET_SOURCE_DEEPPS_GPIO:
        case RESET_SOURCE_DEEPPS_RTC:
            bk_value = TUYA_RESET_REASON_DEEPSLEEP;
            break;
            
        case RESET_SOURCE_CRASH_XAT0:
        case RESET_SOURCE_CRASH_UNDEFINED:
        case RESET_SOURCE_CRASH_PREFETCH_ABORT:
        case RESET_SOURCE_CRASH_DATA_ABORT:
        case RESET_SOURCE_CRASH_UNUSED:
        case RESET_SOURCE_CRASH_PER_XAT0:
            bk_value = TUYA_RESET_REASON_FAULT;
            break;

        default:
            bk_value = TUYA_RESET_REASON_UNKNOWN;
            break;

    }

    return bk_value;
}

/**
* @brief Get a random number in the specified range
*
* @param[in] range: range
*
* @note This API is used for getting a random number in the specified range
*
* @return a random number in the specified range
*/
INT_T tkl_system_get_random(CONST UINT_T range)
{
    unsigned int trange = range;

    if (range == 0) {
        trange = 0xFF;
    }

    static char exec_flag = FALSE;

    if (!exec_flag) {
        exec_flag = TRUE;
    }

    return (bk_rand() % trange);
}

