#include "tkl_gpio.h"

#include "BkDriverGpio.h"
#include "gpio_pub.h"

typedef struct {
    bk_gpio_t           gpio;
    TUYA_GPIO_IRQ_CB     cb;
    void                *args;
} pin_dev_map_t;

static pin_dev_map_t pinmap[] = {
    {GPIO0,  NULL, NULL}, {GPIO1,  NULL, NULL}, {GPIO2,  NULL, NULL}, {GPIO3,  NULL, NULL},
    {GPIO4,  NULL, NULL}, {GPIO5,  NULL, NULL}, {GPIO6,  NULL, NULL}, {GPIO7,  NULL, NULL},
    {GPIO8,  NULL, NULL}, {GPIO9,  NULL, NULL}, {GPIO10, NULL, NULL}, {GPIO11, NULL, NULL},
    {GPIO12, NULL, NULL}, {GPIO13, NULL, NULL}, {GPIO14, NULL, NULL}, {GPIO15, NULL, NULL},
    {GPIO16, NULL, NULL}, {GPIO17, NULL, NULL}, {GPIO18, NULL, NULL}, {GPIO19, NULL, NULL},
    {GPIO20, NULL, NULL}, {GPIO21, NULL, NULL}, {GPIO22, NULL, NULL}, {GPIO23, NULL, NULL},
    {GPIO24, NULL, NULL}, {GPIO25, NULL, NULL}, {GPIO26, NULL, NULL}, {GPIO27, NULL, NULL},
    {GPIO28, NULL, NULL}, {GPIO29, NULL, NULL}, {GPIO30, NULL, NULL}, {GPIO31, NULL, NULL},
    {GPIO32,  NULL, NULL}, {GPIO33,  NULL, NULL}, {GPIO34,  NULL, NULL}, {GPIO35,  NULL, NULL},
    {GPIO36,  NULL, NULL}, {GPIO37,  NULL, NULL}, {GPIO38,  NULL, NULL}, {GPIO39,  NULL, NULL}
};

#define PIN_DEV_CHECK_ERROR_RETURN(__PIN, __ERROR)                          \
    if (__PIN >= sizeof(pinmap)/sizeof(pinmap[0])) {                        \
        return __ERROR;                                                     \
    }

/**
 * @brief gpio init
 * 
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:  gpio config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, CONST TUYA_GPIO_BASE_CFG_T *cfg)
{
    bk_gpio_config_t   bk_gpio_cfg;
    //! set pin direction
    switch (cfg->direct)
    {
        case TUYA_GPIO_INPUT:
            if(cfg->mode == TUYA_GPIO_PULLUP)
            {
                bk_gpio_cfg = INPUT_PULL_UP;
            }
            else if(cfg->mode == TUYA_GPIO_PULLDOWN)
            {
                bk_gpio_cfg = INPUT_PULL_DOWN;
            }
            else 
            {
                bk_gpio_cfg = INPUT_NORMAL;
            }
            break;
        case TUYA_GPIO_OUTPUT:
            bk_gpio_cfg = OUTPUT_NORMAL;
            break;
        default:
            return OPRT_NOT_SUPPORTED;
    }
    BkGpioInitialize(pinmap[pin_id].gpio, bk_gpio_cfg);

    //! set pin init level
    if (TUYA_GPIO_OUTPUT == cfg->direct) {
        if(TUYA_GPIO_LEVEL_LOW == cfg->level)
        {
            BkGpioOutputLow(pinmap[pin_id].gpio);
        }
        else
        {
            BkGpioOutputHigh(pinmap[pin_id].gpio);
        }
    }

    return OPRT_OK;
}

/**
 * @brief gpio deinit
 * 
 * @param[in] pin_id: gpio pin id
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_deinit(TUYA_GPIO_NUM_E pin_id)
{
    return OPRT_OK;
}

/**
 * @brief gpio write
 * 
 * @param[in] pin_id: gpio pin id
 * @param[in] level: gpio output level value
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_write(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    if (level == TUYA_GPIO_LEVEL_HIGH) {
        BkGpioOutputHigh(pinmap[pin_id].gpio);
    } else {
        BkGpioOutputLow(pinmap[pin_id].gpio);
    }

    return OPRT_OK;
}

/**
 * @brief gpio read
 * 
 * @param[in] pin_id: gpio pin id
 * @param[out] level: gpio output level
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_read(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E *level)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    if( BkGpioInputGet(pinmap[pin_id].gpio) )
    {
        *level = TUYA_GPIO_LEVEL_HIGH;
    }
    else
    {
        *level = TUYA_GPIO_LEVEL_LOW;
    }
    return OPRT_OK;
}

/**
 * @brief gpio irq cb
 * NOTE: this API will call irq callback
 * 
 * @param[in] arg: gpio pin id
 * @return none
 */
static void __tkl_gpio_irq_cb(void *arg)
{
    TUYA_GPIO_NUM_E pin_id = (TUYA_GPIO_NUM_E)arg;
    if(pinmap[pin_id].cb){
        pinmap[pin_id].cb(pinmap[pin_id].args);
    }
    return;
}


/**
 * @brief gpio irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:  gpio irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_init(TUYA_GPIO_NUM_E pin_id, CONST TUYA_GPIO_IRQ_T *cfg)
{
    bk_gpio_irq_trigger_t trigger;
    if(pin_id >= 40){
        return OPRT_NOT_SUPPORTED;
    }
    if(cfg == NULL){
        return OPRT_INVALID_PARM;
    }

    switch (cfg->mode)
    {
        case TUYA_GPIO_IRQ_RISE:
            trigger = IRQ_TRIGGER_RISING_EDGE;
            break;
        case TUYA_GPIO_IRQ_FALL:
            trigger = IRQ_TRIGGER_FALLING_EDGE;
            break;
        case TUYA_GPIO_IRQ_LOW:
            trigger = IRQ_TRIGGER_LOW_LEVEL;
            break;
        case TUYA_GPIO_IRQ_HIGH:
            trigger = IRQ_TRIGGER_HGIH_LEVEL;
            break;
        default:
            return OPRT_NOT_SUPPORTED;
    }

    pinmap[pin_id].cb = cfg->cb;
    pinmap[pin_id].args = cfg->arg;
    BkGpioEnableIRQ(pinmap[pin_id].gpio, trigger, (bk_gpio_irq_handler_t)__tkl_gpio_irq_cb, NULL);

    return OPRT_OK;
}

/**
 * @brief gpio irq enable
 * 
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_enable(TUYA_GPIO_NUM_E pin_id)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    return OPRT_OK;
}

/**
 * @brief gpio irq disable
 * 
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_disable(TUYA_GPIO_NUM_E pin_id)
{
    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    BkGpioDisableIRQ(pinmap[pin_id].gpio);
    return OPRT_OK;
}



#if 1  //tkl_gpio_test 
#include "tkl_thread.h"

#define TEST_PIN1 14     //output
#define TEST_PIN2 15    //input 
#define TEST_PIN3 16     //irq

static void pin_irq_cb(void *arg)
{
    tkl_log_output("pin %d irq\r\n",TEST_PIN3);
}

static tkl_gpio_irq_test(void)
{
    int value = 4567;
    TUYA_GPIO_IRQ_T cfg  = {
        .mode = TUYA_GPIO_IRQ_RISE,
        .cb = pin_irq_cb,
        .arg = (void*)value
    };
    tkl_gpio_irq_init(TEST_PIN3,&cfg);
}

static TKL_THREAD_HANDLE thread ;

static void gpio_test_thread(void *arg) 
{
    TUYA_GPIO_BASE_CFG_T cfg1 = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = TUYA_GPIO_LEVEL_HIGH
    };

    TUYA_GPIO_BASE_CFG_T cfg2 = {
        .mode = TUYA_GPIO_PULLUP,
        .direct = TUYA_GPIO_INPUT
    }; 

    tkl_gpio_init(TEST_PIN1, &cfg1);
    tkl_gpio_init(TEST_PIN2, &cfg2);    
    tkl_gpio_irq_test();

    int time = 0;
    TUYA_GPIO_LEVEL_E level;
    while(1) {
        tkl_gpio_read(TEST_PIN2,&level);
        tkl_log_output("pin[%d] state: %s\r\n", TEST_PIN3, level ? "high" : "low");
        tkl_gpio_write(TEST_PIN1,TUYA_GPIO_LEVEL_HIGH);
        tkl_system_sleep(500);
        tkl_gpio_write(TEST_PIN1,TUYA_GPIO_LEVEL_LOW);
        tkl_system_sleep(500);
        time ++;
        if(time == 10) {
            tkl_log_output("disable gpio irq\r\n");
            tkl_gpio_irq_disable(TEST_PIN3);
            tkl_gpio_deinit(TEST_PIN3);
        }
    }
}

//gpio test 入口函数
void tkl_gpio_test(void) 
{
    if(OPRT_OK != tkl_thread_create(&thread, "gpio_test", 1024, 0, gpio_test_thread, NULL)) {
        tkl_log_output("create gpio_test_thread failed\r\n");
        return ;
    }
    tkl_log_output("=====%s=======\r\n",__func__);
    tkl_log_output("pin[%d] output, pin[%d] input, pin[%d] irq\r\n",TEST_PIN1, TEST_PIN2, TEST_PIN3);
}

#else 
void tkl_gpio_test(void) 
{}
#endif



