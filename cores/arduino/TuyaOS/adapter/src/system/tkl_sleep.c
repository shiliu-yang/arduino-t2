#include "tkl_sleep.h"
#include "wlan_ui_pub.h"


unsigned char cpu_lp_flag = 0;


/**
* @brief Set the low power mode of CPU
*
* @param[in] enable: enable switch
* @param[in] mode:   cpu sleep mode
*
* @note This API is used for setting the low power mode of CPU.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_cpu_sleep_mode_set(BOOL_T enable, TUYA_CPU_SLEEP_MODE_E mode)
{
    os_printf("*******************************tuya_os_adapt_set_cpu_lp_mode,en = %d, mode = %d\r\n",enable,mode);

    if(mode == TUYA_CPU_SLEEP) {
        if(enable) {
            if(cpu_lp_flag == 0) {
                cpu_lp_flag = 1;                
                os_printf("pmu_release_wakelock(PMU_OS)\r\n");
            }

            bk_wlan_mcu_ps_mode_enable();
            os_printf("bk_wlan_mcu_ps_mode_enable()\r\n");
        }else {
            bk_wlan_mcu_ps_mode_disable();
            os_printf("bk_wlan_mcu_ps_mode_disable()\r\n");
        }
    }else {
        return OPRT_OS_ADAPTER_CPU_LPMODE_SET_FAILED;
    }

    return OPRT_OK;
}


