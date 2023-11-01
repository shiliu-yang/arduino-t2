#include "tkl_hci.h"
#include "hci.h"

#if !CFG_USE_BK_HOST
extern void ble_entry(void);

OPERATE_RET tkl_hci_init(VOID)
{
    hci_driver_open();
    ble_entry();

    return OPRT_OK;
}

OPERATE_RET tkl_hci_deinit(VOID)
{
    bk_printf("\r\ntkl_hci_deinit\r\n");
    return OPRT_OK;
}

OPERATE_RET tkl_hci_reset(VOID)
{
    bk_printf("\r\ntkl_hci_reset\r\n");
    return OPRT_OK;
}

OPERATE_RET tkl_hci_cmd_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
    return hci_driver_send(HCI_CMD_MSG_TYPE, buf_len, (UCHAR_T *)p_buf);
}

OPERATE_RET tkl_hci_acl_packet_send(CONST UCHAR_T *p_buf, USHORT_T buf_len)
{
    return hci_driver_send(HCI_ACL_MSG_TYPE, buf_len, (UCHAR_T *)p_buf);
}

OPERATE_RET tkl_hci_callback_register(CONST TKL_HCI_FUNC_CB hci_evt_cb, CONST TKL_HCI_FUNC_CB acl_pkt_cb)
{
    bk_printf("\r\ntkl_hci_callback_register\r\n");
    hci_set_event_callback((hci_func_evt_cb)hci_evt_cb, (hci_func_evt_cb)acl_pkt_cb);
    return OPRT_OK;
}
#endif
