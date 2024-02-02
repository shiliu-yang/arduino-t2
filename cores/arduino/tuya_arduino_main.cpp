/**
 * @file tuya_arduino_main.cpp
 * @author www.tuya.com
 * @brief tuya_arduino_main module is used to 
 * @version 0.1
 * @date 2024-02-02
 *
 * @copyright Copyright (c) tuya.inc 2024
 *
 */

#include "tuya_cloud_types.h"
#include "tuya_iot_com_api.h"
#include "tuya_cloud_wifi_defs.h"
#include "tal_thread.h"
#include "tal_system.h"
#include "tal_log.h"
#include "tkl_uart.h"

#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
#include "lwip_init.h"
#endif

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE arduino_thrd_hdl = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/

#if (defined(__cplusplus)||defined(c_plusplus))
extern "C"{
#endif

STATIC void arduino_thread(void *arg)
{
    extern void setup();
    extern void loop();
    setup();

    for (;;) {
        loop();
        tal_system_sleep(1);
    }

    return;
}

STATIC void tuya_app_thread(void *arg)
{
    OPERATE_RET rt = OPRT_OK;

    /* Initialization LWIP first!!! */
#if defined(ENABLE_LWIP) && (ENABLE_LWIP == 1)
    TUYA_LwIP_Init();
#endif

    TY_INIT_PARAMS_S init_param = {0};
    init_param.init_db = TRUE;
    strcpy(init_param.sys_env, TARGET_PLATFORM);
    TUYA_CALL_ERR_LOG(tuya_iot_init_params(NULL, &init_param));

    tal_log_set_manage_attr(TAL_LOG_LEVEL_DEBUG);

    // wait rf cali
    extern char get_rx2_flag(void);
    while (get_rx2_flag() == 0) {
        tal_system_sleep(1);
    }

    THREAD_CFG_T thrd_param = {1024*10, THREAD_PRIO_3, (CHAR_T *)"arduino_thrd"};
    tal_thread_create_and_start(&arduino_thrd_hdl, NULL, NULL, arduino_thread, NULL, &thrd_param);

    return;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {4096, THREAD_PRIO_3, (CHAR_T *)"tuya_app_thread"};
    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}

/* stub for __libc_init_array */
void _fini(void) {}
void _init(void)
{
    ;
}

// This hack is needed because Beken SDK is not linking against libstdc++ correctly.
void * __dso_handle = 0;

#if (defined(__cplusplus)||defined(c_plusplus))
}
#endif
