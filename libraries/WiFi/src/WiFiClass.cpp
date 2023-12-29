#include "WiFiClass.h"

extern "C" {
#include <string.h>
#include <stdlib.h>
#include "tal_wifi.h"
}//extern "C"

WiFiClass::WiFiClass()
{
}

WiFiClass::~WiFiClass()
{
}

uint8_t WiFiClass::channel(void)
{
    uint8_t channel = 0;

    tal_wifi_get_cur_channel(&channel);

    return channel;
}

bool WiFiClass::mode(WiFiMode_t mode)
{
    OPERATE_RET rt = OPRT_OK;

    rt = tal_wifi_set_work_mode(mode);

    return (rt == OPRT_OK) ? true : false;
}

WiFiMode_t WiFiClass::getMode()
{
    WF_WK_MD_E mode = WWM_UNKNOWN;

    tal_wifi_get_work_mode(&mode);

    return mode;
}

bool WiFiClass::enableSTA(bool enable)
{
    OPERATE_RET rt = OPRT_OK;

    if (enable) {
        rt = tal_wifi_set_work_mode(WWM_STATION);
    } else {
        rt = tal_wifi_set_work_mode(WWM_POWERDOWN);
    }

    return (rt == OPRT_OK) ? true : false;
}

bool WiFiClass::enableAP(bool enable)
{
    OPERATE_RET rt = OPRT_OK;

    if (enable) {
        rt = tal_wifi_set_work_mode(WWM_SOFTAP);
    } else {
        rt = tal_wifi_set_work_mode(WWM_POWERDOWN);
    }

    return (rt == OPRT_OK) ? true : false;
}

bool WiFiClass::setSleep(bool enabled)
{
    OPERATE_RET rt = OPRT_OK;

    if (enabled) {
        rt = tal_wifi_lp_enable();
    } else {
        rt = tal_wifi_lp_disable();
    }

    return (rt == OPRT_OK) ? true : false;
}

IPAddress WiFiClass::localIP(void)
{
    OPERATE_RET rt = OPRT_OK;
    IPAddress retIP((uint32_t)0);
    NW_IP_S staIP;

    // TODO: fast connect can't get ip
    rt = tal_wifi_get_ip(WF_STATION, &staIP);
    if (OPRT_OK == rt) {
        retIP = IPAddress((const char *)staIP.ip);
    }
    return retIP;
}

/*********************************** wifi station *******************************************/
int WiFiClass::begin(const char* ssid)
{
    return begin(ssid, nullptr);
}

int WiFiClass::begin(const char* ssid, const char *password)
{
    WF_STATION_STAT_E rtVal = WSS_IDLE;
    OPERATE_RET rt = OPRT_OK;
    WiFiMode_t workMode = getMode();

    if (workMode != WIFI_STA) {
        mode(WIFI_STA);
    }

    rt = tal_wifi_station_connect((SCHAR_T *)ssid, (SCHAR_T *)password);
    if (rt != OPRT_OK) {
        tal_wifi_station_get_status(&rtVal);
    }

    return (int)rtVal;
}

bool WiFiClass::connected()
{
    OPERATE_RET rt = OPRT_OK;
    WF_STATION_STAT_E rtVal = WSS_IDLE;

    rt = tal_wifi_station_get_status(&rtVal);
    if (rt != OPRT_OK) {
        return false;
    }

    return (rtVal == WSS_CONN_SUCCESS) ? true : false;
}

bool WiFiClass::isConnected()
{
    return connected();
}

bool WiFiClass::disconnect()
{
    OPERATE_RET rt = OPRT_OK;
    WiFiMode_t workMode = getMode();

    if (workMode == WIFI_AP) {
        rt = tal_wifi_ap_stop();
    } else if (workMode == WIFI_STA) {
        rt = tal_wifi_station_disconnect();
    } else if (workMode == WIFI_AP_STA) {
        // nothing todo
    } else {
        return false;
    }

    return (rt == OPRT_OK) ? true : false;
}

/*********************************** wifi softAP *******************************************/
bool WiFiClass::beginAP(const char *ssid)
{
    return beginAP(ssid, nullptr, 1);
}

bool WiFiClass::beginAP(const char *ssid, uint8_t channel)
{
    return beginAP(ssid, nullptr, channel);
}

bool WiFiClass::beginAP(const char *ssid, const char* password)
{
    return beginAP(ssid, password, 1);
}

bool WiFiClass::beginAP(const char *ssid, const char* password, uint8_t channel)
{
    OPERATE_RET rt = OPRT_OK;
    WF_AP_CFG_IF_S apCfg;
    WiFiMode_t workMode = getMode();

    memset(&apCfg, 0, sizeof(apCfg));
    apCfg.s_len = strlen(ssid);
    if (apCfg.s_len > WIFI_SSID_LEN || apCfg.s_len == 0) {
        return false;
    }

    if (password != nullptr) {
        apCfg.p_len = strlen(password);
        if (apCfg.p_len > WIFI_PASSWD_LEN) {
            return false;
        }
    }

    strncpy((char *)apCfg.ssid, ssid, WIFI_SSID_LEN);
    if (password != nullptr) {
        strncpy((char *)apCfg.passwd, password, WIFI_PASSWD_LEN);
        apCfg.md = WAAM_WPA2_PSK;
    } else {
        apCfg.md = WAAM_OPEN;
    }
    apCfg.chan = channel;
    apCfg.ssid_hidden = 0;
    apCfg.max_conn = 3;
    apCfg.ms_interval = 100;
    strncpy((char *)apCfg.ip.gw, "192.168.1.1", 16);
    strncpy((char *)apCfg.ip.ip, "192.168.1.123", 16);
    strncpy((char *)apCfg.ip.mask, "255.255.255.0", 16);

    if (workMode != WIFI_AP) {
        mode(WIFI_AP);
    }

    rt = tal_wifi_ap_start(&apCfg);

    return (rt == OPRT_OK) ? true : false;
}

bool WiFiClass::softAPdisconnect()
{
    return disconnect();
}

WiFiClass WiFi;
