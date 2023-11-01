#include "tkl_timer.h"

#include "BkDriverTimer.h"

/* private macros */
#define TIMER_DEV_NUM       4

/* private variables */
static TUYA_TIMER_BASE_CFG_E timer_map[] = {
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL}
};

/* extern function */
extern OSStatus bk_timer_read_cnt(uint8_t timer_id, uint32_t *timer_cnt);
extern OSStatus bk_timer_initialize_us(uint8_t timer_id, uint32_t time_us, void *callback);


/**
 * @brief timer cb
 * 
 * @param[in] args: hw timer id
 *
 * @return none
 */
static void __tkl_hw_timer_cb(void *args)
{
    TUYA_TIMER_NUM_E timer_id = (TUYA_TIMER_NUM_E)args;
    if(timer_map[timer_id].cb){
        timer_map[timer_id].cb(timer_map[timer_id].args);
    }
    return;
}


/**
 * @brief timer init
 * 
 * @param[in] timer_id timer id
 * @param[in] cfg timer configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_init(TUYA_TIMER_NUM_E timer_id, TUYA_TIMER_BASE_CFG_E *cfg)
{
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }
    if(cfg == NULL){
        return OPRT_INVALID_PARM;
    }
    if ((2 == timer_id) || (3 == timer_id)) {//bk timer2、3 被系统占用了实际 bk timerID 为 timer4~timer5
        timer_id +=2;
    }
    timer_map[timer_id].mode = cfg->mode;
    timer_map[timer_id].cb = cfg->cb;
    timer_map[timer_id].args = cfg->args;
    return OPRT_OK;
}

/**
 * @brief timer start
 * 
 * @param[in] timer_id timer id
 * @param[in] us when to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_start(TUYA_TIMER_NUM_E timer_id, UINT_T us)
{
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    if ((0 == timer_id) || (1 == timer_id)) {
        bk_timer_initialize_us(timer_id, us, __tkl_hw_timer_cb);
    } else {
        if (us < 1000) {
            /* tuya timer2~timer3 can't not set cycle less than 1ms */
            return OPRT_INVALID_PARM;
        }
        bk_timer_initialize((timer_id + 2), us / 1000, __tkl_hw_timer_cb); //bk timer2、3 被系统占用了实际 bk timerID 为 timer4~timer5
        return OPRT_OK;
    }

    return OPRT_OK;
}

/**
 * @brief timer stop
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_stop(TUYA_TIMER_NUM_E timer_id)
{
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    if ((0 == timer_id) || (1 == timer_id)) {
        bk_timer_stop(timer_id);
    } else {
        bk_timer_stop(timer_id + 2); //bk timer2、3 被系统占用了实际 bk timerID 为 timer4~timer5
        return OPRT_OK;
    }

    return OPRT_OK;
}

/**
 * @brief timer deinit
 * 
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_deinit(TUYA_TIMER_NUM_E timer_id)
{
    return OPRT_OK;
}

/**
 * @brief timer get
 * 
 * @param[in] timer_id timer id
 * @param[out] ms timer interval
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get(TUYA_TIMER_NUM_E timer_id, UINT_T *us)
{
    uint32_t count;

    if ((0 == timer_id) || (1 == timer_id)) {
        bk_timer_read_cnt(timer_id, &count);

        if (us != NULL) {
            *us = count / 26;
        }
    } 
    else {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

