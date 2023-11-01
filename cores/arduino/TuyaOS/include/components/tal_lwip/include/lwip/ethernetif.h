/**
 * @file ethernetif.h
 * @brief LWIP网口相关封装
 *
 * @copyright copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */
#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "lwip/netif.h"
#include "tuya_hal_network.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
/* num of netif: 0 is to STATION wifi interface, 1 is to AP wifi interface */

typedef enum {
    NETIF_STA_IDX = 0,
    NETIF_AP_IDX,
    NETIF_NUM
} TUYA_NETIF_TYPE;

typedef struct {
    ip4_addr_t ip;
    ip4_addr_t netmask;
    ip4_addr_t gw;
} ty_netif_ip_info_s;

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/

/**
 * @brief get netif by index
 *
 * @param[in]       net_if_idx    the num of netif index
 * @return  NULL: get netif fail   other: the point of netif
 */
struct netif *tuya_ethernetif_get_netif_by_index(const TUYA_NETIF_TYPE net_if_idx);

/**
 * @brief set netif ipaddr from lwip   (没有调试)
 *
 * @param[in]       net_if_idx    index of netif
 * @param[out]      ip            ip of netif(ip gateway mask)
 * @return  void
 */
void tuya_ethernetif_broadcast_set(const TUYA_NETIF_TYPE net_if_idx, bool_t enable);

/**
 * @brief set netif ipaddr from lwip   (没有调试)
 *
 * @param[in]       net_if_idx    index of netif
 * @param[out]      ip            ip of netif(ip gateway mask)
 * @return  void
 */
void tuya_ethernetif_set_ip(const TUYA_NETIF_TYPE net_if_idx, NW_IP_S *ip);

/**
 * @brief get netif ipaddr from lwip
 *
 * @param[in]       net_if_idx    index of netif
 * @param[out]      ip            ip of netif(ip gateway mask)
 * @return  void
 */
void tuya_ethernetif_get_ip(const TUYA_NETIF_TYPE net_if_idx, NW_IP_S *ip);

/**
 * @brief set netif's mac
 *
 * @param[in]       net_if_idx    index of netif
 * @param[in]       mac           mac to set
 * @return  int    OPRT_OS_ADAPTER_OK:success   other:fail
 */
int tuya_ethernetif_mac_set(const TUYA_NETIF_TYPE net_if_idx, NW_MAC_S *mac);

/**
 * @brief get netif's mac
 *
 * @param[in]       net_if_idx    index of netif
 * @param[out]      mac           mac to set
 * @return  int    OPRT_OS_ADAPTER_OK:success   other:fail
 */
int tuya_ethernetif_mac_get(const TUYA_NETIF_TYPE net_if_idx, NW_MAC_S *mac);

/**
 * @brief netif check(check netif is up/down and ip is valid)
 *
 * @param   void
 * @return  int    OPRT_OS_ADAPTER_OK:netif is up and ip is valid
 */
//int tuya_ethernetif_station_state_get(void);

/**
 * @brief ethernet interface recv the packet
 *
 * @param[in]      netif       the netif to which to recieve the packet
 * @param[in]      total_len   the length of the packet recieved from the netif
 * @return  void
 */
//int tuya_ethernetif_recv(struct netif *netif, struct pbuf *p);

/**
 * @brief ethernet interface sendout the pbuf packet
 *
 * @param[in]      netif     the netif to which to be inited
 * @return  err_t  SEE "err_enum_t" in "lwip/err.h" to see the lwip err(ERR_OK: SUCCESS other:fail)
 */
err_t tuya_ethernetif_init(struct netif *netif);


//unsigned int tuya_ethernetif_ip_chksum(void *buf, unsigned short len);

#if LWIP_EAPOL_SUPPORT
extern int tuya_hostap_eapol_input(int vif_index, unsigned char *buf, unsigned short len);
#endif /* LWIP_EAPOL_SUPPORT */

int tuya_ethernetif_get_ifindex_by_mac(NW_MAC_S *mac, TUYA_NETIF_TYPE *net_if_idx);

#endif /* __ETHERNETIF_H__ */
