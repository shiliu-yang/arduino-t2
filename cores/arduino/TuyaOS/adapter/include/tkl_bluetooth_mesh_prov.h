#ifndef __TKL_BLUETOOTH_MESH_H__
#define __TKL_BLUETOOTH_MESH_H__

#include "tkl_bluetooth_mesh_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Function for initializing the mesh provisioner
 * @param   None
 * @return  SUCCESS              Initialized successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_provisioner_init(void);

/**
 * @brief   Refer to many project, BLE-ONLY mode will affect the mesh, after the mesh affecting, we need to reset.
 * @param   config_reset:       0: Only Reset Mesh Stack; 1: Reset Stack And reset mesh configurations
 * @return  SUCCESS             Successfully mesh reset procedure.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_stack_reset(UCHAR_T config_reset);

/**
 * @brief   Register Mesh Event Callback
 * @param   TKL_MESH_EVT_FUNC_CB Refer to @TKL_MESH_EVT_FUNC_CB
 * @return  SUCCESS              Register successfully.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_callback_register(CONST TKL_MESH_EVT_FUNC_CB mesh_evt);

/**
 * @brief   We need to set mesh info, include netkey and appkey
 * @param   [in] local_info:    set netkey/appkey and local node address.
 * @return  SUCCESS             Successfully mesh info set.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_info_set(TKL_MESH_LOCAL_INFO_T local_info);

/**
 * @brief   [Mesh Provisioner] Try to get unprovisioned beacon. Scan-Opearations is always running, we just need to get
 *          the unprovisioned beacon or not. The real parameters is given from CHIP.
 * @param :
 *          [in] enable:        0:disable scan unprovisioned beacon. 1: scan unprovisioned beacon
 *          [in] timeout:       unit: ms.
 * @return  SUCCESS             Successfully initiated scanning procedure.
 *          
 * @note    the unprovisioned beacon is given through TKL_MESH_EVT_ADV_REPORT event !!!!!!
 */
OPERATE_RET tkl_ble_mesh_prov_scan(UCHAR_T enable, UINT_T timeout);

/**
 * @brief   [Mesh Provisioner]Start to provision one mesh device,(Only Provision Invite)
 * @param   [in] device_addr: assign device address to one mesh device.
 *          [in] uuid16: device uuid
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_mesh_adv_provision_invite(TKL_MESH_DEVICE_INFO_T device);

/**
 * @brief   Add Device key into local database
 * @param   [in] device_addr: Node Address
 *          [in] devkey: device key
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_mesh_node_devkey_add(USHORT_T device_addr, UCHAR_T devkey[16]);

/**
 * @brief   Delete Device key from local database
 * @param   [in] device_addr: Node Address
 *          [in] devkey: device key
 * @return  SUCCESS
 *          ERROR
 * */ 
OPERATE_RET tkl_ble_mesh_node_devkey_delete(USHORT_T device_addr, UCHAR_T devkey[16]);

/**
 *@brief    Get seq number.
 *@param    [out] seq : current sequence number
 *@param    [out] iv_index : current IV Index
 *@param    [out] flags : IV Update Flag
 *@return   SUCCESS: success, 
 *          ERROR: failure
 */
OPERATE_RET tkl_ble_mesh_device_get_seq(UINT_T *seq, UINT_T *iv);

/**
 *@brief    Set seq number.
 *@param    [out] seq : sequence number
 *@return   SUCCESS: success, 
 *          ERROR: failure
 */
OPERATE_RET tkl_ble_mesh_device_set_seq(UINT_T seq);

/**
 *@brief    Update IV index
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : IV Update Flag:
 *          0: Normal operation 1: IV Update active
 *@return   SUCCESS: success
 *          ERROR: failure
 */
OPERATE_RET tkl_ble_mesh_device_update_iv_info(UINT_T iv_index, UCHAR_T flags);

/**
 *@brief    set ttl.
 *@param    [in] ttl : ttl number
 *@return   0: success, negetive value: failure
 */
OPERATE_RET tkl_ble_mesh_local_node_set_ttl(UCHAR_T ttl);

/**
 *@brief    set local provisioner network transmit parameters.
 *@param    [in] count : The number of transmissions is the Transmit Count + 1, 
 *                      For example,  if a count value of 0 represents a single transmission.
 *                      if a count value of 7 represents 8 transmissions.[4.2.19.1 Network Transmit Count]
 *@param    [in] interval_steps : transmission interval = (Network Retransmit Interval Steps + 1) * 10.[4.2.19.2 Network Transmit Interval Steps]
 *@return   0: success, negetive value: failure
 */
OPERATE_RET tkl_ble_mesh_local_network_transmit_set(UCHAR_T count, UCHAR_T interval_steps);

/**
 * @brief   [Mesh Provisioner] Send data to mesh node
 * @param   [in] device:        config the device info, if we send generic data, we only need to set the node address.
                p_data:         pointer to data, refer to TKL_MESH_DATA_T               
 * @return  SUCCESS             Successfully send model message.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_model_message_send(TKL_MESH_DEVICE_INFO_T device, TKL_MESH_DATA_T *p_data);

/**
 * @brief   [Mesh Provisioner] Send configuration data to mesh node
 * @param   [in] device:        Device Info, data combined with devkey
                p_data:         pointer to data, refer to TKL_MESH_DATA_T
 * @return  SUCCESS             Successfully send configurations message.
 *          ERROR
 *@Note     [Mesh Profile 4.3.2][Foundation Model]
 *          Configuration messages are used to control states that determine network-related behaviors of the node,
 *          manipulate network and application keys, as well as perform other operations that require an elevated
 *          level of security. Every configuration message shall be encrypted and authenticated using a DevKey.
 *          Because DevKeys are unique for every node, configuration messages shall be sent only to unicast
 *          addresses.
 * */
OPERATE_RET tkl_ble_mesh_config_message_send(TKL_MESH_DEVICE_INFO_T device, TKL_MESH_DATA_T *p_data);

/**
 * @brief   [Beacon Central]    Send data to beacon devices, and will be sended in mesh-bear layer.
 * @param   [in] device:        Device Info, if you only post tuya-beacon data, the device info can be NULL
 *               p_data:        pointer to data, refer to TKL_MESH_DATA_T
 * @return  SUCCESS             Successfully send beacon data.
 *          ERROR
 * */
OPERATE_RET tkl_ble_mesh_beacon_message_send(TKL_MESH_DEVICE_INFO_T device, TKL_MESH_DATA_T *p_data);

#ifdef __cplusplus
}
#endif

#endif

