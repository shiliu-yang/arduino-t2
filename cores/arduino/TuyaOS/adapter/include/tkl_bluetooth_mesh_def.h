/**
 * @file tkl_bluetooth_mesh_def.h
 * @brief This is tuya tal_adc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_BLUETOOTH_MESH_DEF_H__
#define __TKL_BLUETOOTH_MESH_DEF_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"
#include "tkl_bluetooth.h"

#ifndef _PACKED_
#define _PACKED_  __attribute__ ((packed))
#endif

#ifndef _WEAK_
#define _WEAK_ __attribute__((weak))
#endif


/**@brief mesh access msg parameters. */
typedef struct {
    UINT_T    opcode;        /**< Mesh opcode. */
    UCHAR_T   *data;         /**< Mesh data. */
    USHORT_T  data_len;      /**< Mesh data lenth. */
} TKL_MESH_ACCESS_MSG_T;

/**@brief network parameters. */
typedef struct {
    USHORT_T  src_addr;           /**< Source unicast address. */
    USHORT_T  dst_addr;           /**< Destination  unicast address. */
    UCHAR_T   model_index;        /**< Msg model index. */
    UINT_T    seq;                /**< Sequence num of this msg. */
    UCHAR_T   ttl;                /**< Time To Live. */
    USHORT_T  app_key_index;      /**< The appkey index of this msg ues. */
    USHORT_T  net_key_index;      /**< The networkkey index of this msg ues. */
    CHAR_T    rssi;               /**< used when rx in adv bearer. */
} TKL_MESH_NET_PARAM_T;

/**@brief network provision data. */
typedef struct{
    UCHAR_T   net_key[16];        /**< Network key. */
    USHORT_T  key_index;          /**< Network key index. */
    UCHAR_T   flags;              /**< Network key flag. */
    UCHAR_T   iv_index[4];        /**< Network IV index. */
    USHORT_T  unicast_address;    /**< Node unicast address. */
} _PACKED_ TKL_NET_PROV_DATA_T;

/**@brief app key data. */
typedef struct{
    UCHAR_T   net_app_idx[3];     /**< Network key index. */
    UCHAR_T   app_key[16];        /**< App key. */
} _PACKED_ TKL_APP_KEY_DATA_T;

typedef OPERATE_RET (*TKL_MESH_MSG_RECV_CB)(TKL_MESH_ACCESS_MSG_T *msg_raw, TKL_MESH_NET_PARAM_T *net_param);

/**@brief model handle. */
typedef struct {
    UINT_T                 model_id;        /**< Model identifier. */
    TKL_MESH_MSG_RECV_CB   model_receive;   /**< Model receive callback. */
    USHORT_T               model_handle;    /**< Model handel or model index. */
} TKL_MESH_MODEL_HADLE_T;

/**@brief mesh node feature. */
typedef enum {
    MESH_FEATURE_RELAY = 0x00,  /**< Relay. */
    MESH_FEATURE_PROXY,         /**< Proxy. */
    MESH_FEATURE_FRIEND,        /**< Friend. */
    MESH_FEATURE_LPN,           /**< LPN. */
} MESH_FEATURE_T;

typedef enum {
    TKL_MESH_EVT_STACK_INIT = 0x01,                 /**< Mesh Stack Initial Callback Event */

    TKL_MESH_EVT_STACK_DEINIT,                      /**< Mesh Stack Deinit Callback Event */

    TKL_MESH_EVT_STACK_RESET,                       /**< Mesh Stack Reset Callback Event */

    TKL_MESH_EVT_UNPROV_BEACON_REPORT,              /**< Receive Mesh Unprovisioned Beacon Data */

    TKL_MESH_EVT_PRIVATE_BEACON_REPORT,             /**< Receive Tuya-Private Beacon Data */

    TKL_MESH_EVT_INVITE_EVT,                        /**< Invite Mesh Device Event */

    TKL_MESH_EVT_CONFIG_DATA_RX,                    /**< Receive Mesh Configuration Message */

    TKL_MESH_EVT_MODEL_DATA_RX,                     /**< Receive Mesh Model Message */
} TKL_MESH_EVT_TYPE_E;

typedef struct {
    UCHAR_T     peer_addr[6];                       /**< Mesh Device Mac Address */
    UCHAR_T     peer_uuid[16];                      /**< Mesh Device UUID */

    USHORT_T    node_addr;                          /**< Mesh Node Address */
    UCHAR_T     node_devkey[16];                    /**< Mesh Device key */
}TKL_MESH_DEVICE_INFO_T;

typedef struct {
    USHORT_T    local_addr;                         /**< Mesh Provisioner Local Address */

    UCHAR_T     netkey[16];                         /**< Mesh Provisioner Local Netkey */
    UCHAR_T     appkey[16];                         /**< Mesh Provisioner Local Appkey */
}TKL_MESH_LOCAL_INFO_T;

typedef struct {
    UCHAR_T     mac[6];                             /**< Mesh Provisioner Scan Adv Mac Address */
    UCHAR_T     uuid[16];                           /**< Mesh Provisioner Scan Mesh Device UUID */
    USHORT_T    oob;                                /**< Mesh Provisioner Scan Mesh Device OOB */
    UINT_T      uri_hash;                           /**< Mesh Provisioner Scan Mesh Device URI Hash */
    CHAR_T      rssi;                               /**< Mesh Provisioner Scan Mesh Device Rssi */
}TKL_MESH_UNPROV_BEACON_T;

typedef struct {
    UCHAR_T     mac[6];                             /**< Beacon Central Scan Adv Mac Address while in mesh mode*/
    UCHAR_T     length;                             /**< Beacon Central Scan advertising data length*/
    UCHAR_T     *p_data;                            /**< Beacon Central Scan advertising data */
    CHAR_T      rssi;                               /**< Beacon Central Scan advertising Rssi */
}TKL_PRIVATE_BEACON_T;

typedef struct {
    UINT_T      opcode;                             /**< Mesh opcode. Indicate the mesh data with opcode */
    UCHAR_T     count;                              /**< [Mesh& Gateway Special] The number of transmissions is the Transmit Count + 1 */
    UCHAR_T     interval_steps;                     /**< [Mesh& Gateway Special] Transmission interval = (Network Retransmit Interval Steps + 1) * 10 */

    USHORT_T    data_len;                           /**< Mesh Data Length */
    UCHAR_T     *p_data;                            /**< Pointer For Mesh Data */
}TKL_MESH_DATA_T;

typedef struct {
    UINT_T      opcode;                             /**< Mesh opcode. Point the mesh opcode while receiving data. */
    USHORT_T    node_addr;                          /**< Node Address */

    USHORT_T    data_len;                           /**< Recevie Mesh Data Length */
    UCHAR_T     *p_data;                            /**< Recevie Pointer of Mesh Data */
}TKL_MESH_DATA_RECEIVE_T;

typedef struct {
    USHORT_T    node_addr;                          /**< Assign mesh Node Address */
    UCHAR_T     devkey[16];                         /**< Get Mesh Node Dev-key After Provision*/
}TKL_MESH_PROV_T;

typedef struct {
    TKL_MESH_EVT_TYPE_E         type;               /**< Mesh Event Type */
    INT_T                       state;              /**< Mesh Event States */

    union {
        TKL_MESH_UNPROV_BEACON_T unprov_report;     /**< Receive Mesh Unprovisioned Beacon Data */
        TKL_PRIVATE_BEACON_T     beacon_report;     /**< Receive Tuya Private Beacon Data while running beacon cental in mesh mode*/

        TKL_MESH_PROV_T         prov;               /**< Provision Callback State */
        TKL_MESH_DATA_RECEIVE_T config_report;      /**< Report Mesh Model Configuration Data */
        TKL_MESH_DATA_RECEIVE_T model_report;       /**< Report Mesh Model Message Data */
    }mesh_event;
} TKL_MESH_EVT_PARAMS_T;

/**< Define Event Callback for mesh*/
typedef VOID(*TKL_MESH_EVT_FUNC_CB)(TKL_MESH_EVT_PARAMS_T *p_event);

typedef enum{
    TKL_MESH_PROVISION_SUCCESS = 0,
    TKL_MESH_PROVISION_START,
    TKL_MESH_PROVISION_TIMEOUT,
    TKL_MESH_RESET,
    TKL_MESH_RESET_IN_RAM,
    TKL_MESH_REVERT_IN_MESH,
    TKL_MESH_GROUP_SUB_ADD,
    TKL_MESH_GROUP_SUB_DEL,

    TKL_OTA_START,
    TKL_OTA_SUCCESS,
    TKL_OTA_FAIL,
}TKL_MESH_STATE_T;

typedef VOID(*TKL_MESH_NET_STATE_CB_T)(TKL_MESH_STATE_T state);

#endif

