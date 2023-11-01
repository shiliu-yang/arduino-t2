#include "tkl_wifi.h"
#include "tuya_error_code.h"
#include "tkl_semaphore.h"
#include "tkl_queue.h"
#include "tkl_thread.h"
#include "tkl_memory.h"
#include "tkl_output.h"
#include "tkl_system.h"

#include "rw_pub.h"
#include "wlan_ui_pub.h"
#include "uart_pub.h"
#include "ieee802_11_defs.h"
#include "rw_pub.h"


/***********************************************************
*************************micro define***********************
***********************************************************/
#define WIFI_MGNT_FRAME_RX_MSG              (1 << 0)
#define WIFI_MGNT_FRAME_TX_MSG              (1 << 1)

#define SCAN_MAX_AP 64

#define WIFI_CONNECT_ERROR_MAX_CNT          3

/***********************************************************
*************************variable define********************
***********************************************************/
typedef struct {
    unsigned char msg_id;
    unsigned int len;
    unsigned char *data;
}wifi_mgnt_frame;

static WF_WK_MD_E wf_mode = WWM_POWERDOWN; 
static TKL_SEM_HANDLE scanHandle = NULL;
static SNIFFER_CALLBACK snif_cb = NULL;
static WIFI_REV_MGNT_CB mgnt_recv_cb = NULL;
static WIFI_EVENT_CB wifi_event_cb = NULL;
static unsigned char wifi_lp_status = 0;  
static unsigned char mgnt_cb_exist_flag = 0;
static bool lp_mode = FALSE;
static unsigned char first_set_flag = TRUE;

extern unsigned char cpu_lp_flag;

wifi_country_t country_code[] = 
{
    {.cc= "CN", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "US", .schan=1, .nchan=11, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "JP", .schan=1, .nchan=14, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    {.cc= "EU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL},
    //{.cc= "AU", .schan=1, .nchan=13, .max_tx_power=0, .policy=WIFI_COUNTRY_POLICY_MANUAL}
};

static void tkl_wifi_powersave_disable(void);
static void tkl_wifi_powersave_enable(void);

/**
 * @brief set wifi station work status changed callback
 *
 * @param[in]      cb        the wifi station work status changed callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_init(WIFI_EVENT_CB cb)
{
    wifi_event_cb = cb;
    return OPRT_OK;
}


/**
 * @brief 原厂 scan 结束的CB
 * 
 * @param[out] 
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
static void scan_cb(void *ctxt, unsigned char param)
{
    if(scanHandle) {
        tkl_semaphore_post(scanHandle);
    }
}

/**
 * @brief scan current environment and obtain all the ap
 *        infos in current environment
 * 
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 * @return  OPRT_OS_ADAPTER_OK: success  Other: fail
 */
static OPERATE_RET tkl_wifi_all_ap_scan(AP_IF_S **ap_ary, unsigned int *num)
{
    AP_IF_S *item;
    AP_IF_S *array = NULL;
    OPERATE_RET ret;
    INT_T i;
    INT_T scan_cnt;
    INT_T ssid_len;
    ScanResult_adv apList;

    if((NULL == ap_ary) || (NULL == num) || NULL != scanHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    ret = tkl_semaphore_create_init(&scanHandle, 0, 1);
    if (ret !=  OPRT_OK) {
        return ret;
    }

    mhdr_scanu_reg_cb(scan_cb, 0);
    if (bk_wlan_start_scan() == 0) {
        ret = tkl_semaphore_wait(scanHandle, 5000);
        bk_printf("wait sem timeout\r\n");
    } else {
        ret = OPRT_COM_ERROR;
        bk_printf("start scan failed\r\n");
    }    
    tkl_semaphore_release(scanHandle);
    scanHandle = NULL;
    
    if (ret !=  OPRT_OK) {
        return ret;
    }
    
    if ((wlan_sta_scan_result(&apList) != 0) || (0 == apList.ApNum)) {
        tkl_log_output("scan err\r\n");
        goto SCAN_ERR;
    }

    scan_cnt = apList.ApNum;

    if (scan_cnt > SCAN_MAX_AP) {
        scan_cnt = SCAN_MAX_AP;
    }

    if(0 == scan_cnt) {
        goto SCAN_ERR;
    }
    
    array = ( AP_IF_S *)tkl_system_malloc(SIZEOF(AP_IF_S) * scan_cnt);
    if(NULL == array){
        goto SCAN_ERR;
    }
    
	tkl_system_memset(array, 0, (sizeof(AP_IF_S) * scan_cnt));
    for (i = 0; i < scan_cnt; i++) {
        item = &array[i];

        item->channel = apList.ApList[i].channel;
        item->rssi = apList.ApList[i].ApPower;
        os_memcpy(item->bssid, apList.ApList[i].bssid, 6);

        ssid_len = os_strlen(apList.ApList[i].ssid);
        if (ssid_len > WIFI_SSID_LEN) {
            ssid_len = WIFI_SSID_LEN;
        }

        memset(item->ssid, 0, ssid_len);
        os_strncpy((char *)item->ssid, apList.ApList[i].ssid, ssid_len);
		item->s_len = ssid_len;
        item->security = apList.ApList[i].security;
        
    }
    
    *ap_ary = array;
    *num = scan_cnt & 0xff;
    if (apList.ApList != NULL) {
        tkl_system_free(apList.ApList);
    }
    return  OPRT_OK;

SCAN_ERR:
    if (apList.ApList != NULL) {
        tkl_system_free(apList.ApList);
        apList.ApList = NULL;
    }

    if(array) {
        tkl_system_free(array);
        array = NULL;
    }
    return OPRT_OS_ADAPTER_COM_ERROR;
}



/**
 * @brief scan current environment and obtain the ap
 *        infos in current environment
 * 
 * @param[in]       ssid        the specific ssid
 * @param[out]      ap_ary      current ap info array
 * @param[out]      num         the num of ar_ary
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 * @note if ssid == NULL means scan all ap, otherwise means scan the specific ssid
 */
OPERATE_RET tkl_wifi_scan_ap(CONST SCHAR_T *ssid, AP_IF_S **ap_ary, UINT_T *num)
{
    if(first_set_flag) {
        extern void extended_app_waiting_for_launch(void); 
        extended_app_waiting_for_launch(); /* 首次等待wifi初始化OK */
        first_set_flag = FALSE;
    }
    
    if(NULL == ssid)
    {
        return tkl_wifi_all_ap_scan(ap_ary, num);
    }

    AP_IF_S *array = NULL;
    OPERATE_RET ret;
    INT_T i = 0, j = 0, ssid_len;
    ScanResult_adv apList;

    tkl_system_memset(&apList, 0, sizeof(ScanResult_adv));

    if((NULL == ssid) || (NULL == ap_ary) ||  NULL != scanHandle) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    ret = tkl_semaphore_create_init(&scanHandle, 0, 1);
    if ( ret !=  OPRT_OK ) {
        return ret;
    }
    
    mhdr_scanu_reg_cb(scan_cb, 0);
    if (bk_wlan_start_assign_scan((UINT8 **)&ssid, 1) == 0) {
        ret = tkl_semaphore_wait(scanHandle, 5000);
        bk_printf("wait sem timeout\r\n");
    } else {
        ret = OPRT_COM_ERROR;
        bk_printf("start scan failed\r\n");
    }    
    tkl_semaphore_release(scanHandle);
    scanHandle = NULL;
    
    if (ret !=  OPRT_OK) {
        return ret;
    }

    if ((wlan_sta_scan_result(&apList) != 0) || (0 == apList.ApNum)) {
        tkl_log_output("scan err\r\n");
        goto SCAN_ERR;
    }       
    
    array = (AP_IF_S *)tkl_system_malloc(sizeof(AP_IF_S));
    if(NULL == array) {
        goto SCAN_ERR;
    }
    
    tkl_system_memset(array, 0, sizeof(AP_IF_S));
    array->rssi = -100;

    for (i = 0; i < apList.ApNum ; i++) {
        /* skip non-matched ssid */
        if (os_strcmp(apList.ApList[i].ssid, (char *)ssid))
            continue;

        /* found */
        if (apList.ApList[i].ApPower < array->rssi) { /* rssi 信号强度比较 */
            continue;
        }
        array->channel = apList.ApList[i].channel;
        array->rssi = apList.ApList[i].ApPower;
        os_memcpy(array->bssid, apList.ApList[i].bssid, 6);

        ssid_len = os_strlen(apList.ApList[i].ssid);
        if (ssid_len > WIFI_SSID_LEN) {
            ssid_len = WIFI_SSID_LEN;
        }

        memset(array->ssid, 0, ssid_len);
        os_strncpy((char *)array->ssid, apList.ApList[i].ssid, ssid_len);
        array->s_len = ssid_len;
        array->security = apList.ApList[i].security;
        
		j++;
    }

	if (j == 0) {
        goto SCAN_ERR;
    }
    *ap_ary = array;
    if (apList.ApList != NULL) {
        tkl_system_free(apList.ApList);
    }

    return OPRT_OK;
    
SCAN_ERR:
    if (apList.ApList != NULL) {
        tkl_system_free(apList.ApList);
        apList.ApList = NULL;
    }

    if (array) {
        tkl_system_free(array);
        array = NULL;
    }
    return OPRT_OS_ADAPTER_COM_ERROR;
}

/**
 * @brief release the memory malloced in <tkl_wifi_ap_scan>
 *        if needed. tuyaos will call this function when the 
 *        ap info is no use.
 * 
 * @param[in]       ap          the ap info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_release_ap(AP_IF_S *ap)
{
    if(NULL == ap) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    tkl_system_free(ap);
    ap = NULL;

    return OPRT_OK;
}

/**
 * @brief receive wifi management 接收到数据原厂的回调处理函数,将数据放入到queue中
 * @param[in]       *frame
 * @param[in]       len 
 * @param[in]       *param 
 * @return  none
 */
static void wifi_mgnt_frame_rx_notify(unsigned char *frame, int len, void *param)
{
    if(mgnt_recv_cb) {
         mgnt_recv_cb((unsigned char *)frame, len);
    }
}

/**
 * @brief start a soft ap
 * 
 * @param[in]       cfg         the soft ap config
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_start_ap(CONST WF_AP_CFG_IF_S *cfg)
{
    int ret = OPRT_OK;
    network_InitTypeDef_ap_st wNetConfig;

    tkl_system_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_ap_st));

    switch ( cfg->md ) {
        case WAAM_OPEN :
            wNetConfig.security = SECURITY_TYPE_NONE;
            break;
        case WAAM_WEP :
            wNetConfig.security = SECURITY_TYPE_WEP;
            break;
        case WAAM_WPA_PSK :
            wNetConfig.security = SECURITY_TYPE_WPA2_TKIP;
            break;
        case WAAM_WPA2_PSK :
            wNetConfig.security = SECURITY_TYPE_WPA2_AES;
            break;
        case WAAM_WPA_WPA2_PSK:
            wNetConfig.security = SECURITY_TYPE_WPA2_MIXED;
            break;
        default:
            ret = OPRT_INVALID_PARM;
            break;
    }

    bk_printf("ap net info ip: %s, mask: %s, gw: %s\r\n", cfg->ip.ip, cfg->ip.mask, cfg->ip.gw);
    if (OPRT_OK == ret) {
        wNetConfig.channel = cfg->chan;
        wNetConfig.dhcp_mode = DHCP_SERVER;
        tkl_system_memcpy((char *)wNetConfig.wifi_ssid, cfg->ssid, cfg->s_len);
        tkl_system_memcpy((char *)wNetConfig.wifi_key, cfg->passwd, cfg->p_len);

        os_strcpy((char *)wNetConfig.local_ip_addr, cfg->ip.ip);
        os_strcpy((char *)wNetConfig.net_mask, cfg->ip.mask);
        os_strcpy((char *)wNetConfig.gateway_ip_addr, cfg->ip.gw);
        os_strcpy((char *)wNetConfig.dns_server_ip_addr, cfg->ip.gw);

        bk_printf("ssid:%s, key:%s, channel: %d\r\n", wNetConfig.wifi_ssid, wNetConfig.wifi_key, wNetConfig.channel);
        ret = bk_wlan_start_ap_adv(&wNetConfig);
        if(OPRT_OK != ret) {
            tkl_log_output("start ap err\r\n");
        }
    }

    return ret;
}

/**
 * @brief stop a soft ap
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_stop_ap(VOID_T)
{
    bk_wlan_stop(SOFT_AP);
    return OPRT_OK;
}

/**
 * @brief set wifi interface work channel
 * 
 * @param[in]       chan        the channel to set
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_cur_channel(CONST UCHAR_T chan)
{
   int status;
        
    if ((chan > 14) || (chan < 1)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    if (mgnt_recv_cb != NULL) { /* 为了避免路由器断开之后，管理祯接收NG的问题 */            
        tkl_log_output("mgnt recv cb<<\r\n");
        if (HW_IDLE == nxmac_current_state_getf()) {
            tkl_wifi_powersave_disable();     /* 强制退出低功耗的模式 */
            tkl_log_output("mm active~~\r\n");
            rw_msg_send_mm_active_req();//强制激活mac控制层
        }
        bk_wlan_reg_rx_mgmt_cb((mgmt_rx_cb_t)wifi_mgnt_frame_rx_notify, 2);    /* 将回调注册到原厂中 */
    }
    
    
    status = bk_wlan_set_channel(chan);
    if (status) {
        return OPRT_OS_ADAPTER_CHAN_SET_FAILED;
    } else {
        return OPRT_OK;
    }

}

/**
 * @brief get wifi interface work channel
 * 
 * @param[out]      chan        the channel wifi works
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_cur_channel(UCHAR_T *chan)
{   
    *chan = (unsigned char)bk_wlan_get_channel();  
    return OPRT_OK;
}

static void _wf_sniffer_set_cb(unsigned char *data, int len, hal_wifi_link_info_t *info)
{
    if (NULL != snif_cb) {
        (*snif_cb)(data, len, info->rssi);
    }
}

/**
 * @brief enable / disable wifi sniffer mode.
 *        if wifi sniffer mode is enabled, wifi recv from
 *        packages from the air, and user shoud send these
 *        packages to tuya-sdk with callback <cb>.
 * 
 * @param[in]       en          enable or disable
 * @param[in]       cb          notify callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_sniffer(CONST BOOL_T en, CONST SNIFFER_CALLBACK cb)
{
     if(en) {
        WF_WK_MD_E mode = WWM_POWERDOWN;
        tkl_wifi_get_work_mode(&mode);
        if(( mode == WWM_SOFTAP) || (mode == WWM_STATIONAP) ){
            bk_wlan_set_ap_monitor_coexist(1);
        }else {
            bk_wlan_set_ap_monitor_coexist(0);
        }
        
        snif_cb = cb;
        bk_wlan_register_monitor_cb(_wf_sniffer_set_cb);
        
        bk_wlan_start_monitor();
       
    }else {
        bk_wlan_stop_monitor();
        bk_wlan_register_monitor_cb(NULL);        
    }
    return OPRT_OK;
}

/**
 * @brief get wifi ip info.when wifi works in
 *        ap+station mode, wifi has two ips.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      ip          the ip addr info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_ip(CONST WF_IF_E wf, NW_IP_S *ip)
{
    int ret = OPRT_OK;
    IPStatusTypedef wNetConfig;
    WiFi_Interface iface;

    tkl_system_memset(&wNetConfig, 0x0, sizeof(IPStatusTypedef));

    switch ( wf ) {
        case WF_STATION:
            iface = STATION;
            wNetConfig.dhcp = DHCP_CLIENT;
        break;
    
        case WF_AP:
            iface = SOFT_AP;
            wNetConfig.dhcp = DHCP_SERVER;
        break;
        
        default:
            ret = OPRT_OS_ADAPTER_INVALID_PARM;
        break;
    }

    if (OPRT_OK == ret) {
        bk_wlan_get_ip_status(&wNetConfig, iface);
        os_strcpy(ip->ip, wNetConfig.ip);
        os_strcpy(ip->mask, wNetConfig.mask);
        os_strcpy(ip->gw, wNetConfig.gate);
    }

    return ret;  
}

/**
 * @brief set wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[in]       mac         the mac info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_mac(CONST WF_IF_E wf, CONST NW_MAC_S *mac)
{
    if(wifi_set_mac_address((char *)mac))
    {
        return OPRT_OK;
    } else {
        return OPRT_OS_ADAPTER_MAC_SET_FAILED;
    }
}

/**
 * @brief get wifi mac info.when wifi works in
 *        ap+station mode, wifi has two macs.
 * 
 * @param[in]       wf          wifi function type
 * @param[out]      mac         the mac info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_mac(CONST WF_IF_E wf, NW_MAC_S *mac)
{
   if(WF_STATION == wf) {
        wifi_get_mac_address((char *)mac,2);
   }else{
        wifi_get_mac_address((char *)mac,1);
   }
    
    return OPRT_OK;
}

static OPERATE_RET _wf_wk_mode_exit(WF_WK_MD_E mode)
{
    int ret = OPRT_OK;
    switch(mode) {
        case WWM_POWERDOWN : 
            break;

        case WWM_SNIFFER: 
            bk_wlan_stop_monitor();
            break;

        case WWM_STATION: 
            bk_wlan_stop(STATION);
            break;

        case WWM_SOFTAP: 
            bk_wlan_stop(SOFT_AP);
            break;

        case WWM_STATIONAP: 
            bk_wlan_stop(SOFT_AP);
            bk_wlan_stop(STATION);
            break;

        default:
            break;
    }

    return ret;
}

/**
 * @brief set wifi work mode
 * 
 * @param[in]       mode        wifi work mode
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_work_mode(CONST WF_WK_MD_E mode)
{
    OPERATE_RET ret = OPRT_OK;
    WF_WK_MD_E current_mode;

    if(first_set_flag) {
        extern void extended_app_waiting_for_launch(void); 
        extended_app_waiting_for_launch(); /* 首次等待wifi初始化OK */
        first_set_flag = FALSE;
    }

    ret = tkl_wifi_get_work_mode(&current_mode);
    if((OPRT_OK == ret) && (current_mode != mode)) {
        _wf_wk_mode_exit(current_mode);
    }
    wf_mode = mode;

    return ret;
}

/**
 * @brief get wifi work mode
 * 
 * @param[out]      mode        wifi work mode
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_work_mode(WF_WK_MD_E *mode)
{
    *mode = wf_mode;
    return OPRT_OK;
}

/**
 * @brief : get ap info for fast connect
 * @param[out]      fast_ap_info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_connected_ap_info(FAST_WF_CONNECTED_AP_INFO_T **fast_ap_info)
{
    //TODO
    return OPRT_COM_ERROR;
}

/**
 * @brief get wifi bssid
 * 
 * @param[out]      mac         uplink mac
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_get_bssid(UCHAR_T *mac)
{
    int ret = OPRT_OK;
    LinkStatusTypeDef sta;

    if(NULL == mac) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    ret = bk_wlan_get_link_status(&sta);
    if (OPRT_OK == ret) {
        tkl_system_memcpy(mac, sta.bssid, 6);
    }
    
    return ret;   
}

/**
 * @brief set wifi country code
 * 
 * @param[in]       ccode  country code
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_country_code(CONST COUNTRY_CODE_E ccode)
{
    int ret = OPRT_OK;
    int country_num = sizeof(country_code) / sizeof(wifi_country_t);
    wifi_country_t *country = (ccode < country_num) ? &country_code[ccode] : &country_code[COUNTRY_CODE_CN];

    ret = bk_wlan_set_country(country);
    if (ret != OPRT_OK) {
        tkl_log_output("set country err!\r\n");
    }

    if (ccode == COUNTRY_CODE_EU) { //enable
        bk_wlan_phy_open_cca();
    } else { //disable
        bk_wlan_phy_close_cca();
    }

    return OPRT_OK;
}

/**
 * @brief do wifi calibration
 *
 * @note called when test wifi
 *
 * @return true on success. faile on failure
 */
extern int manual_cal_rfcali_status(void);
BOOL_T tkl_wifi_set_rf_calibrated(VOID_T)
{
    int stat = manual_cal_rfcali_status();

    if(stat) {
        return true;
    }

    return false;
}

/**
 * @brief 进入低功耗处理
 * 
 * @param[in] none
 * @return  none
 */
static void tkl_wifi_powersave_enable(void)
{
    bk_wlan_stop_ez_of_sta();
    rtos_delay_milliseconds(50);
    bk_wlan_dtim_rf_ps_mode_enable();
    bk_wlan_dtim_rf_ps_timer_start();
    //lp_status = 1;
}

/**
 * @brief 退出低功耗处理
 * 
 * @param[in] none
 * @return  none
 */
static void tkl_wifi_powersave_disable(void)
{
    bk_wlan_dtim_rf_ps_mode_disable();
    bk_wlan_dtim_rf_ps_timer_pause();
    rtos_delay_milliseconds(50);
    bk_wlan_start_ez_of_sta();
    //lp_status = 0;
}


/**
 * @brief set wifi lowpower mode
 * 
 * @param[in]       enable      enbale lowpower mode
 * @param[in]       dtim     the wifi dtim
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_set_lp_mode(CONST BOOL_T enable, CONST UCHAR_T dtim)
{
    if(TRUE == enable) {
        if(wifi_lp_status) {
            tkl_log_output("bk wlan ps mode has enable\r\n");
            return OPRT_OK;
        }

        if(mgnt_recv_cb || mgnt_cb_exist_flag) {
            tkl_wifi_powersave_enable();
            if(!mgnt_recv_cb) {
                mgnt_cb_exist_flag = 0;
            }
        } else {
            bk_wlan_dtim_rf_ps_mode_enable();
            bk_wlan_dtim_rf_ps_timer_start();
        }
        tkl_log_output("bk_ps_mode_enable\r\n");
    }else {
        if(mgnt_recv_cb || mgnt_cb_exist_flag) {
            tkl_wifi_powersave_disable();
            if(!mgnt_recv_cb) {
                mgnt_cb_exist_flag = 0;
            }
        }
       // bk_wlan_mcu_ps_mode_disable();
        tkl_log_output("bk_ps_mode_disable\r\n");
    }

    wifi_lp_status = enable;
    return OPRT_OK; 
}

/**
 * @brief : fast connect
 * @param[in]      fast_ap_info
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_fast_connect(CONST FAST_WF_CONNECTED_AP_INFO_T *fast_ap_info)
{
    //快联在原厂在普联接口中实现，这里无需实现
    return OPRT_COM_ERROR;
}

STATIC TKL_THREAD_HANDLE wifi_state_get_thread = NULL;
static void ty_wifi_state_get_thread( void *arg )
{
    int stat = RW_EVT_STA_IDLE, last_stat = RW_EVT_STA_IDLE;
    while(1){
        static unsigned long s_connect_err = 0;

        stat = mhdr_get_station_status();
        if(stat != last_stat && wifi_event_cb) {
            switch (stat) {
                case RW_EVT_STA_GOT_IP:
                    s_connect_err = 0;
                    bk_printf("ty_wifi_state_get_thread: WFE_CONNECTED %d\r\n", stat);
                    wifi_event_cb(WFE_CONNECTED, NULL);
                    last_stat = stat;
                    break;

                case RW_EVT_STA_CONNECT_FAILED:
                case RW_EVT_STA_NO_AP_FOUND:
                case RW_EVT_STA_ASSOC_FULL:
                    s_connect_err++;
                    if (s_connect_err == WIFI_CONNECT_ERROR_MAX_CNT) {
                        bk_printf("ty_wifi_state_get_thread: RW_EVT_STA_CONNECT_FAILED %d\r\n", stat);
                        wifi_event_cb(WFE_CONNECT_FAILED, NULL);
                    }
                    last_stat = stat;
                    break;
                case RW_EVT_STA_PASSWORD_WRONG:
                case RW_EVT_STA_DHCP_FAILED:
                    bk_printf("ty_wifi_state_get_thread: RW_EVT_STA_CONNECT_FAILED %d\r\n", stat);
                    wifi_event_cb(WFE_CONNECT_FAILED, NULL);
                    last_stat = stat;
                    break;
                

                case RW_EVT_STA_DISCONNECTED:
                case RW_EVT_STA_BEACON_LOSE:
                    s_connect_err = 0;
                    bk_printf("ty_wifi_state_get_thread: WFE_DISCONNECTED %d\r\n", stat);
                    wifi_event_cb(WFE_DISCONNECTED, NULL);
                    last_stat = stat;
                    break;

                case RW_EVT_STA_SCANNING:
                    last_stat = stat;
                    break;

                default:
                    break;
            }
        } 
        tkl_system_sleep(100);
    }
}

/**
 * @brief connect wifi with ssid and passwd
 * 
 * @param[in]       ssid
 * @param[in]       passwd
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_connect(CONST SCHAR_T *ssid, CONST SCHAR_T *passwd)
{
    int ret = OPRT_COM_ERROR;
    network_InitTypeDef_st wNetConfig;
    tkl_system_memset(&wNetConfig, 0x0, sizeof(network_InitTypeDef_st));

    if(wifi_state_get_thread == NULL)
        tkl_thread_create(&wifi_state_get_thread, "wifi_state_get_thread", 1024, 4, ty_wifi_state_get_thread, NULL);

    os_strcpy((char *)wNetConfig.wifi_ssid, (const char *)ssid);
    os_strcpy((char *)wNetConfig.wifi_key, (const char *)passwd);

    wNetConfig.wifi_mode = STATION;
    wNetConfig.dhcp_mode = DHCP_CLIENT;

    ret = bk_wlan_start(&wNetConfig);
    return ret;   
}

/**
 * @brief disconnect wifi from connect ap
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_disconnect(VOID_T)
{
    //bk_wlan_stop(STATION); /* 不需要实现，由于在start中一开始就会停止 */
    return OPRT_OK;
}

/**
 * @brief get wifi connect rssi
 * 
 * @param[out]      rssi        the return rssi
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_conn_ap_rssi(SCHAR_T *rssi)
{
    int ret = OPRT_OK;
    SHORT_T tmp_rssi = 0, sum_rssi = 0;
    SCHAR_T max_rssi = -128, min_rssi = 127;
    LinkStatusTypeDef sta = {0};
    int i = 0, error_cnt = 0;

    for (i = 0; i < 5; i++) {
        memset(&sta, 0x00, sizeof(sta));
        ret = bk_wlan_get_link_status(&sta);
        if (OPRT_OK == ret) {
            tmp_rssi = sta.wifi_strength;
            if (tmp_rssi > max_rssi) {
                max_rssi = tmp_rssi;
            }
            if (rssi[i] < min_rssi) {
                min_rssi = tmp_rssi;
            }
            bk_printf("get rssi: %d\r\n", tmp_rssi);
        } else {
            bk_printf("get rssi error\r\n");
            error_cnt++;
        }
        sum_rssi += tmp_rssi;
        tkl_system_sleep(10);
    }
    if (error_cnt > 2) {
        ret = OPRT_COM_ERROR;
    } else {
        *rssi = (sum_rssi - min_rssi - max_rssi)/(3 - error_cnt);
    }

    return ret;
}

/**
 * @brief get wifi station work status
 * 
 * @param[out]      stat        the wifi station work status
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_station_get_status(WF_STATION_STAT_E *stat)
{
    static unsigned char flag = FALSE;
    rw_evt_type type = mhdr_get_station_status();
    {
        switch (type) {
            case RW_EVT_STA_IDLE:
                *stat = WSS_IDLE;
                break;
            case RW_EVT_STA_CONNECTING:
                *stat = WSS_CONNECTING;
                break;
            case RW_EVT_STA_PASSWORD_WRONG:
                *stat = WSS_PASSWD_WRONG;
                break;
            case RW_EVT_STA_NO_AP_FOUND:
                *stat = WSS_NO_AP_FOUND;
                break;
            case RW_EVT_STA_ASSOC_FULL:
                *stat = WSS_CONN_FAIL;
                break;
            case RW_EVT_STA_BEACON_LOSE:
                *stat = WSS_CONN_FAIL;
                break;
            case RW_EVT_STA_DISCONNECTED:
                *stat = WSS_CONN_FAIL;
                break;
            case RW_EVT_STA_CONNECT_FAILED:
                *stat = WSS_CONN_FAIL;
                break;
            case RW_EVT_STA_CONNECTED:
                *stat = WSS_CONN_SUCCESS;
                break;
            case RW_EVT_STA_GOT_IP:
                *stat = WSS_GOT_IP;
                break;
            default:
                break;
        }
    }

    if((!flag) && (*stat == WSS_GOT_IP)) {
        if (mgnt_recv_cb != NULL) { /* 为了避免路由器断开之后，管理祯接收NG的问题 */
            tkl_wifi_powersave_disable();                              /* 强制退出低功耗的模式 */
            bk_wlan_reg_rx_mgmt_cb((mgmt_rx_cb_t)wifi_mgnt_frame_rx_notify, 2);    /* 将回调注册到原厂中 */
            if (HW_IDLE == nxmac_current_state_getf()) {
                rw_msg_send_mm_active_req();//强制激活mac控制层
            }
        } else {
            bk_wlan_dtim_rf_ps_mode_enable();
            bk_wlan_dtim_rf_ps_timer_start();
        }

        if(cpu_lp_flag & wifi_lp_status) {
            bk_wlan_mcu_ps_mode_enable();
            lp_mode = TRUE;
        } else {
            lp_mode = FALSE;
        }
        
       

        flag = TRUE;
    } else if (*stat != WSS_GOT_IP) {
        flag = FALSE;
        
    }

    return OPRT_OK;
}

/**
 * @brief send wifi management
 * 
 * @param[in]       buf         pointer to buffer
 * @param[in]       len         length of buffer
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_send_mgnt(CONST UCHAR_T *buf, CONST UINT_T len)
{
    int ret = bk_wlan_send_80211_raw_frame((unsigned char *)buf, len);
    if(ret < 0) {
        return OPRT_OS_ADAPTER_MGNT_SEND_FAILED;
    }
    
    return OPRT_OK; 
}

/**
 * @brief register receive wifi management callback
 * 
 * @param[in]       enable
 * @param[in]       recv_cb     receive callback
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_wifi_register_recv_mgnt_callback(CONST BOOL_T enable, CONST WIFI_REV_MGNT_CB recv_cb)
{
    tkl_log_output("recv mgnt callback enable %d, recv_cb: %p\r\n",enable, recv_cb);
    if (enable) {
        WF_WK_MD_E mode;
        int ret = tkl_wifi_get_work_mode(&mode);
        if (OPRT_OK != ret) {
            tkl_log_output("recv mgnt, get mode err\r\n");
            return OPRT_OS_ADAPTER_COM_ERROR;
        }

        if ((mode == WWM_POWERDOWN) || (mode == WWM_SNIFFER)) {
            tkl_log_output("recv mgnt, but mode is err\r\n");
            return OPRT_OS_ADAPTER_COM_ERROR;
        }
        tkl_wifi_powersave_disable();        /* 强制退出低功耗的模式 */
        mgnt_recv_cb = recv_cb;
        bk_wlan_reg_rx_mgmt_cb((mgmt_rx_cb_t)wifi_mgnt_frame_rx_notify, 2); /* 将回调注册到原厂中 */
        if (HW_IDLE == nxmac_current_state_getf()) {
            rw_msg_send_mm_active_req();//强制激活mac控制层
        }
		
        mgnt_cb_exist_flag = 1;

    }else {
        bk_wlan_reg_rx_mgmt_cb(NULL, 2);
        mgnt_recv_cb = NULL;
    }
    return OPRT_OK;
}





