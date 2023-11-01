/**
* @file tkl_watchdog.c
* @brief Common process - adapter the watchdog api
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/
#include "tkl_watchdog.h"

#include "drv_model_pub.h"
#include "wdt_pub.h"
#include "BkDriverWdg.h"


UINT_T tkl_watchdog_init(TUYA_WDOG_BASE_CFG_T *cfg)
{
    if (cfg->interval_ms > 60000) {
        cfg->interval_ms = 60000;
    }

    bk_wdg_initialize((uint32_t)cfg->interval_ms);
    return cfg->interval_ms;
}

OPERATE_RET tkl_watchdog_deinit(VOID_T)
{
    bk_wdg_finalize();
    return OPRT_OK;
}

OPERATE_RET tkl_watchdog_refresh(VOID_T)
{
    bk_wdg_reload();
    return OPRT_OK;
}
