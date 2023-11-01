#include "Arduino.h"

#include "tkl_adc.h"
#include "tkl_pwm.h"

int analogRead(pin_size_t pinNumber)
{
    int readValue = 0;
    UINT8_T chanList = 0;

    TUYA_ADC_NUM_E adcId;
    TUYA_ADC_BASE_CFG_T adcCfg;

    switch (pinNumber) {
        case p22: {
            adcId = ADC_NUM_0;
            chanList = 4; // channel 4
            adcCfg.ch_list = &chanList;
            adcCfg.ch_nums = 1;
            adcCfg.width = 12;
            adcCfg.mode = TUYA_ADC_CONTINUOUS;
            adcCfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
            adcCfg.conv_cnt = 1;
        } break;
        case p23: {
            adcId = ADC_NUM_0;
            chanList = 2;
            adcCfg.ch_list = &chanList;
            adcCfg.ch_nums = 1;
            adcCfg.width = 12;
            adcCfg.mode = TUYA_ADC_CONTINUOUS;
            adcCfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
            adcCfg.conv_cnt = 1;
        } break;
        case p24: {
            adcId = ADC_NUM_0;
            chanList = 1;
            adcCfg.ch_list = &chanList;
            adcCfg.ch_nums = 1;
            adcCfg.width = 12;
            adcCfg.mode = TUYA_ADC_CONTINUOUS;
            adcCfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
            adcCfg.conv_cnt = 1;
        } break;
        case p26: {
            adcId = ADC_NUM_0;
            chanList = 0;
            adcCfg.ch_list = &chanList;
            adcCfg.ch_nums = 1;
            adcCfg.width = 12;
            adcCfg.mode = TUYA_ADC_CONTINUOUS;
            adcCfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
            adcCfg.conv_cnt = 1;
        } break;
        case p28: {
            adcId = ADC_NUM_0;
            chanList = 3;
            adcCfg.ch_list = &chanList;
            adcCfg.ch_nums = 1;
            adcCfg.width = 12;
            adcCfg.mode = TUYA_ADC_CONTINUOUS;
            adcCfg.type = TUYA_ADC_INNER_SAMPLE_VOL;
            adcCfg.conv_cnt = 1;
        } break;
        default : return 0;
    }

    tkl_adc_init(adcId, &adcCfg);

    tkl_adc_read_single_channel(adcId, chanList, &readValue);

    return readValue;
}

void analogReference(uint8_t mode)
{
    return;
}

#define DEFAULT_PWM_FREQ    1000

// value: 0-100
void analogWrite(pin_size_t pinNumber, int value)
{
    TUYA_PWM_NUM_E pwmNum = PWM_NUM_MAX;
    TUYA_PWM_BASE_CFG_T pwmCfg;

    switch (pinNumber) {
        case p6: {
            pwmNum = PWM_NUM_0;
        } break;
        case p7: {
            pwmNum = PWM_NUM_1;
        } break;
        case p8: {
            pwmNum = PWM_NUM_2;
        } break;
        case p9: {
            pwmNum = PWM_NUM_3;
        } break;
        case p24: {
            pwmNum = PWM_NUM_4;
        } break;
        case p26: {
            pwmNum = PWM_NUM_5;
        } break;
        default : return;
    }

    pwmCfg.frequency = DEFAULT_PWM_FREQ;
    pwmCfg.polarity = TUYA_PWM_NEGATIVE;
    pwmCfg.duty = (value*100) % 10000; // tuya duty 0-10000
    tkl_pwm_init(pwmNum, &pwmCfg);
    tkl_pwm_start(pwmNum);

    return;
}
