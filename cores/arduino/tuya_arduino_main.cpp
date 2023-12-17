#include "Arduino.h"

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

#if (defined(__cplusplus)||defined(c_plusplus))
extern "C"{
#endif

STATIC THREAD_HANDLE ty_app_thread = NULL;
STATIC THREAD_HANDLE arduino_thrd_hdl = NULL;

STATIC void arduino_thread(void *arg)
{
    setup();

    for (;;) {
        loop();
        tal_system_sleep(1);
    }

    return;
}

void bk_printf(const char *fmt, ...);
extern char get_rx2_flag(void);

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

#if (defined(__cplusplus)||defined(c_plusplus))
}
#endif

/* stub for __libc_init_array */
extern "C" void _fini(void) {}
extern "C" void _init(void)
{
    ;
}

// This hack is needed because Beken SDK is not linking against libstdc++ correctly.
extern "C" {
void * __dso_handle = 0;
}

