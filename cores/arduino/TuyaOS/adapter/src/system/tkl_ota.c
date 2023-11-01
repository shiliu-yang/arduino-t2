/**
 * @file tkl_ota.c
 * @brief default weak implements of tuya ota, it only used when OS=linux
 * 
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 * 
 */
#include <string.h>
#include "tkl_ota.h"
#include "tuya_error_code.h"
#include "tkl_output.h"
#include "tkl_memory.h"
#include "tkl_flash.h"
#include "tkl_system.h"

#define UG_PKG_HEAD     0x55aa55aa
#define UG_PKG_TAIL     0xaa55aa55
#define UG_START_ADDR   0x12A000   //664k  
#define RT_IMG_WR_UNIT  512
#define BUF_SIZE        4096
#define OTA_MAX_BIN_SIZE (664 * 1024)

typedef enum {
    UGS_RECV_HEADER = 0,
    UGS_RECV_IMG_DATA,
    UGS_FINISH
}UG_STAT_E;
    
typedef struct
{
    unsigned int header_flag;        //0xaa55aa55
    char sw_version[12];             //sofrware version
    unsigned int bin_len;
    unsigned int bin_sum;
    unsigned int head_sum;           //header_flag + sw_version + bin_len + bin_sum
    unsigned int tail_flag;          //0x55aa55aa
}UPDATE_FILE_HDR_S;

typedef struct {
    UPDATE_FILE_HDR_S file_header;
    unsigned int flash_addr;
    unsigned int start_addr;
    unsigned int recv_data_cnt;
    UG_STAT_E stat;
}UG_PROC_S;

/***********************************************************
*************************variable define********************
***********************************************************/
static UG_PROC_S *ug_proc = NULL;
static unsigned char *frist_block_databuf = NULL;
static unsigned char first_block = 1;

extern int tkl_flash_set_protect(const BOOL_T enable);

/**
* @brief ota start notify
*
* @param[in] image_size:  image size
* @param[in] type:        ota type
* @param[in] path:        ota path
*
* @note This API is used for ota start notify
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_ota_start_notify(UINT_T image_size, TUYA_OTA_TYPE_E type, TUYA_OTA_PATH_E path)
{
    if(image_size == 0) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if(ug_proc == NULL) {
        ug_proc = tkl_system_malloc(sizeof(UG_PROC_S));
        if(NULL == ug_proc) {
            return OPRT_MALLOC_FAILED;
        }
    }

    memset(ug_proc,0,sizeof(UG_PROC_S));
    first_block = 1;
    
    return OPRT_OK;;
}

/**
* @brief ota data process
*
* @param[in] pack:       point to ota pack
* @param[in] remain_len: ota pack remain len
*
* @note This API is used for ota data process
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_ota_data_process(TUYA_OTA_DATA_T *pack, UINT_T* remain_len)
{
    unsigned int sum_tmp = 0, i = 0;
    unsigned int write_len = 0;
    unsigned char *buf = NULL;

    
    if(ug_proc == NULL) {
        tkl_log_output("ota don't start or start err,process error!\r\n");
        return OPRT_COM_ERROR;
    }

    switch(ug_proc->stat) {
        case UGS_RECV_HEADER: {
            if(pack->len < sizeof(UPDATE_FILE_HDR_S)) {
                *remain_len = pack->len;
                break;
            }
            
            ug_proc->file_header.tail_flag = (pack->data[28]<<24)|(pack->data[29]<<16)|(pack->data[30]<<8)|pack->data[31];
            ug_proc->file_header.head_sum = (pack->data[24]<<24)|(pack->data[25]<<16)|(pack->data[26]<<8)|pack->data[27];
            ug_proc->file_header.bin_sum = (pack->data[20]<<24)|(pack->data[21]<<16)|(pack->data[22]<<8)|pack->data[23];
            ug_proc->file_header.bin_len = (pack->data[16]<<24)|(pack->data[17]<<16)|(pack->data[18]<<8)|pack->data[19];
            for(i = 0; i < 12; i++) {
                ug_proc->file_header.sw_version[i] = pack->data[4 + i];
            }
            
            ug_proc->file_header.header_flag = (pack->data[0]<<24)|(pack->data[1]<<16)|(pack->data[2]<<8)|pack->data[3];
            
            for(i = 0;i < sizeof(UPDATE_FILE_HDR_S) - 8;i++) {
                sum_tmp += pack->data[i];
            }
            
            tkl_log_output("header_flag(0x%x) tail_flag(0x%x) head_sum(0x%x-0x%x) bin_sum(0x%x)\r\n",ug_proc->file_header.header_flag,ug_proc->file_header.tail_flag,ug_proc->file_header.head_sum,sum_tmp,ug_proc->file_header.bin_sum);

            if((ug_proc->file_header.header_flag !=  UG_PKG_HEAD) || (ug_proc->file_header.tail_flag !=  UG_PKG_TAIL) || (ug_proc->file_header.head_sum != sum_tmp )) {
                memset(&ug_proc->file_header, 0, sizeof(UPDATE_FILE_HDR_S));
                tkl_log_output("bin_file data header err: header_flag(0x%x) tail_flag(0x%x) bin_sum(0x%x) get_sum(0x%x)\r\n",ug_proc->file_header.header_flag,ug_proc->file_header.tail_flag,ug_proc->file_header.head_sum,sum_tmp);
                return OPRT_OS_ADAPTER_OTA_START_INFORM_FAILED;
            }
            
            if(ug_proc->file_header.bin_len >= OTA_MAX_BIN_SIZE) { //ug文件最大为664K
                memset(&ug_proc->file_header, 0, sizeof(UPDATE_FILE_HDR_S));
                tkl_log_output("bin_file too large.... %d\r\n", ug_proc->file_header.bin_len);
                return OPRT_OS_ADAPTER_OTA_PKT_SIZE_FAILED;
            }
            
            tkl_log_output("sw_ver:%s\r\n", ug_proc->file_header.sw_version);
            tkl_log_output("get right bin_file_header!!!\r\n");
            ug_proc->start_addr = UG_START_ADDR;
            ug_proc->flash_addr = ug_proc->start_addr;
            ug_proc->stat = UGS_RECV_IMG_DATA;
            ug_proc->recv_data_cnt = 0;
            *remain_len = pack->len - sizeof(UPDATE_FILE_HDR_S);
            
            tkl_flash_set_protect(FALSE);
            tkl_flash_erase(ug_proc->start_addr,ug_proc->file_header.bin_len);
            tkl_flash_set_protect(TRUE);
            
        } 
        break;
        
        case UGS_RECV_IMG_DATA: {    //dont have set lock for flash! 
            *remain_len = pack->len;
            if((pack->len < RT_IMG_WR_UNIT) && (ug_proc->recv_data_cnt <= (ug_proc->file_header.bin_len - RT_IMG_WR_UNIT))) {
                break;
            }
            
            write_len = pack->len;
            
            while(write_len >= RT_IMG_WR_UNIT) {
                if(first_block) {
                    if(NULL == frist_block_databuf){
                        frist_block_databuf = tkl_system_malloc(RT_IMG_WR_UNIT);
                        if(NULL == frist_block_databuf) {
                            return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
                        }
                    }
                    
                    memcpy(frist_block_databuf, &pack->data[pack->len - write_len], RT_IMG_WR_UNIT);
                    
                    
                    buf = tkl_system_malloc(RT_IMG_WR_UNIT);
                    if(NULL == buf ) {
                        return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
                    }

                    memset(buf, 0xFF , RT_IMG_WR_UNIT);  // make dummy data 
                    tkl_flash_set_protect(FALSE);
                    if(tkl_flash_write(ug_proc->flash_addr, buf, RT_IMG_WR_UNIT)) {
                        tkl_flash_set_protect(TRUE);
                        tkl_log_output("Write sector failed\r\n");
                        if(buf){
                            tkl_system_free(buf);
                            buf = NULL;
                        }
                        return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
                    }
                    tkl_flash_set_protect(TRUE);
                    first_block = 0;    
                } else {
                    tkl_flash_set_protect(FALSE);
                    if(tkl_flash_write(ug_proc->flash_addr, &pack->data[pack->len - write_len], RT_IMG_WR_UNIT)) {
                        tkl_flash_set_protect(TRUE);
                        tkl_log_output("Write sector failed\r\n");
                        if(buf){
                            tkl_system_free(buf);
                            buf = NULL;
                        }
                        return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
                    }
                    tkl_flash_set_protect(TRUE);
                }
                ug_proc->flash_addr += RT_IMG_WR_UNIT;
                ug_proc->recv_data_cnt += RT_IMG_WR_UNIT;
                write_len -= RT_IMG_WR_UNIT; 
                *remain_len = write_len;
            }
            
            if((ug_proc->recv_data_cnt > (ug_proc->file_header.bin_len - RT_IMG_WR_UNIT)) \
                && (write_len >= (ug_proc->file_header.bin_len - ug_proc->recv_data_cnt))) {    //last 512 (write directly when get data )
                
                tkl_flash_set_protect(FALSE);
                
                if(tkl_flash_write(ug_proc->flash_addr, &pack->data[pack->len - write_len], write_len)) {
                    tkl_flash_set_protect(TRUE);
                    tkl_log_output("Write sector failed\r\n");
                    if(buf){
                        tkl_system_free(buf);
                        buf = NULL;
                    }
                    return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
                }
                
                tkl_flash_set_protect(TRUE);
                
                ug_proc->flash_addr += write_len;
                ug_proc->recv_data_cnt += write_len;
                write_len = 0;
                *remain_len = 0;
            }            
            if(ug_proc->recv_data_cnt >= ug_proc->file_header.bin_len) {
                ug_proc->stat = UGS_FINISH;
                if(buf){
                    tkl_system_free(buf);
                    buf = NULL;
                }
                break;
            }
        }
        break;

        case UGS_FINISH: {
            *remain_len = 0;
        }
        break;
    }

    return OPRT_OK;
}

/**
 * @brief firmware ota packet transform success inform, can do verify in this funcion
 * 
 * @param[in]        reset       need reset or not
 * 
 * @return OPRT_OK on success, others on failed
 */
OPERATE_RET tkl_ota_end_notify(BOOL_T reset)
{
    unsigned int i = 0,k = 0,rlen = 0,addr = 0;
    unsigned int flash_checksum = 0;
    unsigned char *pTempbuf = NULL;

    if(ug_proc == NULL) {
        tkl_log_output("ota don't start or start err, can't end inform!\r\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }
    
    pTempbuf = tkl_system_malloc(BUF_SIZE);
    if(NULL == pTempbuf ) {
        tkl_log_output("Malloc failed!!\r\n");
        return OPRT_MALLOC_FAILED;
    }

    
    for(i = 0; i < ug_proc->file_header.bin_len; i += BUF_SIZE) {
        rlen  = ((ug_proc->file_header.bin_len - i) >= BUF_SIZE) ? BUF_SIZE : (ug_proc->file_header.bin_len - i);
        addr = ug_proc->start_addr + i;
        tkl_flash_read(addr, pTempbuf, rlen);
        if(0 == i) {
            memcpy(pTempbuf, frist_block_databuf ,RT_IMG_WR_UNIT); //first 4k block 
        } 
        
        for(k = 0; k < rlen; k++){
            flash_checksum += pTempbuf[k];
        }
     
    }
    
    if(flash_checksum != ug_proc->file_header.bin_sum) {
        tkl_log_output("verify_ota_checksum err  checksum(0x%x)  file_header.bin_sum(0x%x)\r\n",flash_checksum,ug_proc->file_header.bin_sum);
        
        goto OTA_VERIFY_PROC;
    } 
        
    tkl_flash_read(ug_proc->start_addr, pTempbuf, BUF_SIZE);

    tkl_flash_set_protect(FALSE);
    tkl_flash_erase(ug_proc->start_addr, BUF_SIZE);
    tkl_flash_set_protect(TRUE);

    memcpy(pTempbuf, frist_block_databuf, RT_IMG_WR_UNIT); // 还原头部信息的512byte
    tkl_flash_set_protect(FALSE);
    tkl_flash_write(ug_proc->start_addr, pTempbuf, BUF_SIZE);
    tkl_flash_set_protect(TRUE);


    tkl_log_output("the gateway upgrade success\r\n");
    
    tkl_system_free(pTempbuf);
    pTempbuf = NULL;
    
    tkl_system_free(ug_proc);
    ug_proc = NULL;

    tkl_system_free(frist_block_databuf);
    frist_block_databuf = NULL;

    if(TRUE == reset) { //verify 
        tkl_system_reset();
    }
    return OPRT_OK;
    
 OTA_VERIFY_PROC:
    if(pTempbuf != NULL) {
        tkl_system_free(pTempbuf);
        pTempbuf = NULL;
    }

    if(ug_proc != NULL) {
        tkl_system_free(ug_proc);
        ug_proc = NULL;
    }

    if(frist_block_databuf != NULL) {
        tkl_system_free(frist_block_databuf);
        frist_block_databuf = NULL;
    }
    
    return OPRT_OS_ADAPTER_OTA_END_INFORM_FAILED;
}


/**
* @brief get ota ability
*
* @param[out] image_size:  max image size
* @param[out] type:        bit0, 1 - support full package upgrade
                                 0 - dont support full package upgrade
*                          bit1, 1 - support compress package upgrade
                                 0 - dont support compress package upgrade
*                          bit2, 1 - support difference package upgrade
                                 0 - dont support difference package upgrade
* @note This API is used for get chip ota ability
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_ota_get_ability(UINT_T *image_size, TUYA_OTA_TYPE_E *type)
{
    *image_size = OTA_MAX_BIN_SIZE;
    *type = TUYA_OTA_FULL;

    return OPRT_OK;
}
