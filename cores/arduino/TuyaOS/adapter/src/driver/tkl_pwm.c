#include "tkl_pwm.h"

#include "BkDriverGpio.h"
#include "BkDriverPwm.h"

/*
TKL_PWM1_CH6 = GPIO6
*/

#define PWM_DEV_NUM             6

static TUYA_PWM_BASE_CFG_T pwm_cfg[PWM_DEV_NUM] = {{0}};
static unsigned char pwm_start_flag[PWM_DEV_NUM] = {0};

/**
 * @brief pwm init
 * 
 * @param[in] ch_id: pwm channal id
 * @param[in] cfg: pwm config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_init(TUYA_PWM_NUM_E ch_id, CONST TUYA_PWM_BASE_CFG_T *cfg)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }

    pwm_cfg[ch_id].duty = cfg->duty;
    pwm_cfg[ch_id].frequency = cfg->frequency;
    pwm_cfg[ch_id].polarity = cfg->polarity;

    return OPRT_OK;
}

/**
 * @brief pwm deinit
 * 
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_deinit(TUYA_PWM_NUM_E ch_id)
{
    return OPRT_OK;
}

/**
 * @brief pwm start
 * 
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_start(TUYA_PWM_NUM_E ch_id)
{
    int ret = OPRT_OK;
    unsigned int count;
    unsigned int duty;
    
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }

    count = (unsigned int)((26 * 1000000) / pwm_cfg[ch_id].frequency);  // 26M(主频) / 频率 = 计数值
    duty = (unsigned int)(pwm_cfg[ch_id].duty * count / 10000);         // 一个周期的计数值 * 占空比
    if (0 == pwm_start_flag[ch_id]) {
        ret = bk_pwm_initialize(ch_id, count, duty, 0, 0);
        if (kNoErr == ret) {
            ret = bk_pwm_start(ch_id);
        } 
        ret = kNoErr == ret ? OPRT_OK : OPRT_COM_ERROR;
        pwm_start_flag[ch_id] = !ret;
    } else {
        bk_pwm_update_param(ch_id, count, duty, 0, 0);
    }

    return ret;
}

/**
 * @brief multiple pwm channel start
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_start(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    int ret = OPRT_OK;
    unsigned int count;
    unsigned int cold_duty, warm_duty;
    unsigned int dead = 0;
    
    if (2 == num) {
        if ((ch_id[0] > PWM_DEV_NUM) || (ch_id[1] > PWM_DEV_NUM))
        {
            return OPRT_INVALID_PARM;
        }

        if (pwm_cfg[ch_id[0]].frequency != pwm_cfg[ch_id[1]].frequency) {
            return OPRT_COM_ERROR;
        }
        
        count = (unsigned int)((26 * 1000000) / pwm_cfg[ch_id[0]].frequency);       // 26M(主频) / 频率 = 计数值
        cold_duty = (unsigned int)(pwm_cfg[ch_id[0]].duty * count / 10000);         // 一个周期的计数值 * 占空比
        warm_duty = (unsigned int)(pwm_cfg[ch_id[1]].duty * count / 10000);         // 一个周期的计数值 * 占空比
        if (count >= cold_duty + warm_duty) {
            dead = (count - cold_duty - warm_duty) / 2;
        } else {
            bk_printf("cold_duty(%d) + warm_duty(%d) > count(%d)\r\n", cold_duty, warm_duty, count);
            return OPRT_COM_ERROR;
        }
        
        if (0 == pwm_start_flag[ch_id[0]]) {
            ret = bk_pwm_cw_initialize(ch_id[0], ch_id[1], count, cold_duty, warm_duty, dead);
            if (kNoErr == ret) {
                ret = bk_pwm_cw_start(ch_id[0], ch_id[1]);
            } 
            
            ret = kNoErr == ret ? OPRT_OK : OPRT_COM_ERROR;
            pwm_start_flag[ch_id[0]] = !ret;
            pwm_start_flag[ch_id[1]] = !ret;
        } else {
            ret = bk_pwm_cw_update_param(ch_id[0], ch_id[1], count, cold_duty, warm_duty, dead);
            ret = kNoErr == ret ? OPRT_OK : OPRT_COM_ERROR;
        }
        
        return ret;
    } else {
        return OPRT_NOT_SUPPORTED;
    }
}

/**
 * @brief pwm stop
 * 
 * @param[in] ch_id: pwm channal id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_stop(TUYA_PWM_NUM_E ch_id)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }

    bk_pwm_stop(ch_id);
    pwm_start_flag[ch_id] = 0;

    return OPRT_OK;
}

/**
 * @brief multiple pwm channel stop
 *
 * @param[in] ch_id: pwm channal id list
 * @param[in] num  : num of pwm channal to stop
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_multichannel_stop(TUYA_PWM_NUM_E *ch_id, UINT8_T num)
{
    if (2 == num) {
        if ((ch_id[0] > PWM_DEV_NUM) || (ch_id[1] > PWM_DEV_NUM))
        {
            return OPRT_INVALID_PARM;
        }

        bk_pwm_cw_stop(ch_id[0], ch_id[1]);
        pwm_start_flag[ch_id[0]] = 0;
        pwm_start_flag[ch_id[1]] = 0;
        
        return OPRT_OK;
    } else {
        return OPRT_NOT_SUPPORTED;
    }
}

/**
 * @brief pwm duty set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] duty:  pwm duty cycle
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_duty_set(TUYA_PWM_NUM_E ch_id, UINT32_T duty)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }
    
    pwm_cfg[ch_id].duty = duty;
    
    return OPRT_OK;
}

/**
 * @brief pwm frequency set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] frequency: pwm frequency
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_frequency_set(TUYA_PWM_NUM_E ch_id, UINT32_T frequency)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }
    
    pwm_cfg[ch_id].frequency = frequency;
    
    return OPRT_OK;
}

/**
 * @brief pwm polarity set
 * 
 * @param[in] ch_id: pwm channal id, id index starts at 0
 * @param[in] polarity: pwm polarity
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_polarity_set(TUYA_PWM_NUM_E ch_id, TUYA_PWM_POLARITY_E polarity)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief set pwm info
 * 
 * @param[in] ch_id: pwm channal id
 * @param[in] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_set(TUYA_PWM_NUM_E ch_id, CONST TUYA_PWM_BASE_CFG_T *info)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }
    
    pwm_cfg[ch_id].duty = info->duty;
    pwm_cfg[ch_id].frequency = info->frequency;
    pwm_cfg[ch_id].polarity = info->polarity;

    return OPRT_OK;
}

/**
 * @brief get pwm info
 * 
 * @param[in] ch_id: pwm channal id
 * @param[out] info: pwm info
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_pwm_info_get(TUYA_PWM_NUM_E ch_id, TUYA_PWM_BASE_CFG_T *info)
{
    if (ch_id > PWM_DEV_NUM)
    {
        return OPRT_INVALID_PARM;
    }

    info->duty = pwm_cfg[ch_id].duty;
    info->frequency = pwm_cfg[ch_id].frequency;
    info->polarity = pwm_cfg[ch_id].polarity;
    
    return OPRT_OK;
}

