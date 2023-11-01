#include "tkl_adc.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tuya_error_code.h"

#include "gpio_pub.h"
#include "saradc_pub.h"
#include "BkDriverGpio.h"
#include "FreeRTOS.h"
#include "task.h"

/*============================ MACROS ========================================*/
#define ADC_DEV_NUM         1
#define ADC_DEV_CHANNEL_SUM 6

#define ADC_REGISTER_VAL_MAX  4096
#define ADC_VOLTAGE_MAX  2400   //mv

static saradc_desc_t adc_desc = {0};  //ADC结构体
static unsigned char g_adc_init[ADC_DEV_CHANNEL_SUM] = {FALSE};
static unsigned char adc_ch_nums = 0;   // 实际使用的通道数

/**
 * @brief tuya kernel adc init
 * 
 * @param[in] unit_num: adc unit number
 * @param[in] cfg: adc config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_init(TUYA_ADC_NUM_E port_num, TUYA_ADC_BASE_CFG_T *cfg)
{
    unsigned char i = 0;
    unsigned char channel = 0;

    // cfg->ch_list channel number start from 0
    if ((port_num > ADC_DEV_NUM-1) || (cfg->ch_nums > ADC_DEV_CHANNEL_SUM) || (cfg->ch_list == NULL)) {
        tkl_log_output("error port num: %d:%d\r\n", port_num, __LINE__);
        return OPRT_INVALID_PARM;
    }     

    memset(g_adc_init, 0x00, ADC_DEV_CHANNEL_SUM);
    memset(&adc_desc, 0x00, sizeof(adc_desc));

    if (TUYA_ADC_SINGLE == cfg->mode) {
        adc_desc.mode = (ADC_CONFIG_MODE_STEP << 0)
                        | (ADC_CONFIG_MODE_4CLK_DELAY << 2);
    } else if (TUYA_ADC_CONTINUOUS == cfg->mode) {
        adc_desc.mode = (ADC_CONFIG_MODE_CONTINUE << 0)
                        | (ADC_CONFIG_MODE_4CLK_DELAY << 2)
                        | (ADC_CONFIG_MODE_SHOULD_OFF);
    } else {
        return OPRT_INVALID_PARM;
    }
    adc_desc.data_buff_size = cfg->conv_cnt;
    adc_desc.pre_div = 8;
    adc_desc.samp_rate = 0x20;  // bk advise not to change
    adc_desc.p_Int_Handler = saradc_disable;
    
    for (i = 0; i < ADC_DEV_CHANNEL_SUM; i++) {
        channel = cfg->ch_list[i];
        if (channel < ADC_DEV_CHANNEL_SUM) {
            g_adc_init[channel] = TRUE;
        }
    }
    adc_ch_nums = cfg->ch_nums;

    return OPRT_OK;
}

/**
 * @brief adc deinit
 * 
 * @param[in] unit_num: adc unit number

 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_deinit(TUYA_ADC_NUM_E port_num)
{
    return OPRT_OK;
}

/**
 * @brief get adc width
 * 
 * @param[in] unit_num: adc unit number

 *
 * @return adc width
 */
UINT8_T tkl_adc_width_get(TUYA_ADC_NUM_E port_num)
{
    return 12;
}


/**
 * @brief get adc reference voltage
 * 
 * @param[in] NULL

 *
 * @return adc reference voltage(bat: mv)
 */
UINT32_T tkl_adc_ref_voltage_get(TUYA_ADC_NUM_E port_num)
{
    return ADC_VOLTAGE_MAX;
}


/**
 * @brief adc read
 * 
 * @param[in] unit_num: adc unit number
 * @param[out] buff: points to the list of data read from the ADC register
 * @param[out] len:  buff len
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_read_data(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    OPERATE_RET ret = OPRT_OK;
    unsigned char i = 0, j = 0;
    
    if (port_num > ADC_DEV_NUM-1) {
        tkl_log_output("error port num: %d:%d\r\n", port_num, __LINE__);
        return OPRT_INVALID_PARM;
    }
    if (adc_ch_nums * adc_desc.data_buff_size > len) {
        ret = OPRT_COM_ERROR;
    }

    for (i = 0; i < ADC_DEV_CHANNEL_SUM; i++) {
        if (g_adc_init[i]) {
            ret = tkl_adc_read_single_channel(port_num, i, &buff[j*adc_desc.data_buff_size]);
            j++;
        }
    }
    
    return ret;
}

OPERATE_RET tkl_adc_read_single_channel(TUYA_ADC_NUM_E port_num, UINT8_T ch_id, INT32_T *data)
{
    signed char i = 0;
    unsigned int status;
    int adc_hdl;
    unsigned short temp_adc_mv = 0;
    unsigned short temp_result = NULL;
    
    if ((port_num > ADC_DEV_NUM-1) || (ch_id > ADC_DEV_CHANNEL_SUM)) {
        return OPRT_INVALID_PARM;
    }     

    if(!g_adc_init[ch_id]){
        tkl_log_output("adc not init!\r\n");
        return OPRT_OS_ADAPTER_COM_ERROR;
    }

    adc_desc.has_data = 0;
    adc_desc.channel = ch_id + 1;
    adc_desc.pData = (UINT16*)data;
    if(NULL == adc_desc.pData)
        return OPRT_MALLOC_FAILED;
    
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    adc_desc.current_sample_data_cnt = 0;
    adc_desc.current_read_data_cnt = 0;
    adc_hdl = ddev_open(SARADC_DEV_NAME, &status, (unsigned int)&adc_desc); 
    if ((DD_HANDLE_UNVALID == adc_hdl) || (SARADC_SUCCESS != status))
    {
        if (SARADC_SUCCESS != status)
        {
            ddev_close(adc_hdl);
        }
        adc_hdl = DD_HANDLE_UNVALID;
        GLOBAL_INT_RESTORE();
        tkl_log_output("adc ddev_open error:%d\r\n", status);
        
        return OPRT_COM_ERROR;  
    }
    GLOBAL_INT_RESTORE();

    while (adc_desc.all_done == 0)
    {
        i++;
        if (i > 100)
        {
            tkl_log_output("adc value get timeout!\r\n");
            break;
        }
        vTaskDelay(1);
    }
   
    ddev_close(adc_hdl);
    saradc_ensure_close();

    for (i = adc_desc.data_buff_size - 1; i >= 0; i--) {
        temp_result = adc_desc.pData[i];
        temp_adc_mv = (UINT16)(saradc_calculate(temp_result) * 1000);
        temp_result = temp_adc_mv * ADC_REGISTER_VAL_MAX / ADC_VOLTAGE_MAX;
        data[i] = temp_result;
    }

    return OPRT_OK;   
}



/**
 * @brief adc get temperature
 *
 * @return temperature(bat: 'C)
 */
INT32_T tkl_adc_temperature_get(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief read voltage
 *
 * @param[in] port_num: adc port number
 * @param[out] data: convert voltage, voltage range to -vref - +vref
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 */
OPERATE_RET tkl_adc_read_voltage(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    OPERATE_RET ret = OPRT_COM_ERROR;
    UINT32_T ref = tkl_adc_ref_voltage_get(port_num);
    INT32_T i = 0;

    ret = tkl_adc_read_data(port_num, buff, len);
    if (OPRT_OK == ret) {
        for (i = 0; i < len; i++) {
            buff[i] = (buff[i] * ref) / 4096;
        }
    }
    
    return ret;
}

