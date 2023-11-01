#include "tkl_flash.h"

#include "drv_model_pub.h"
#include "flash_pub.h"

typedef struct 
{
    bool            is_start;
    unsigned long   set_ms;
    unsigned long   start;
    unsigned long   current;
} TUYA_OS_STORAGE_TIMER;
//flash 最大持续处理时间
#define FLASH_MAX_HANDLE_KEEP_TIME 10000    //10s

/* TODO: need to consider whether to use locks at the TKL layer*/
extern int hal_flash_lock(void);
extern int hal_flash_unlock(void);

#define PARTITION_SIZE         (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ            PARTITION_SIZE

// flash map 
#define SIMPLE_FLASH_START (0x200000 - 0x3000 - 0xE000)
#define SIMPLE_FLASH_SIZE 0xE000 // 56k

#define SIMPLE_FLASH_SWAP_START (0x200000 - 0x3000)
#define SIMPLE_FLASH_SWAP_SIZE 0x3000 // 12k

#define SIMPLE_FLASH_KEY_ADDR  (0x200000 - 0x3000 - 0xE000 - 0x1000)            //4k

#define UF_PARTITION_START     (0x200000 - 0x3000 - 0xE000 - 0x1000) - 0x3000 - 0x1000 - 0x18000
#define UF_PARTITION_SIZE      0x18000          //96k

#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
    #define PROTECTED_DATA_ADDR (0x200000 - 0x3000 - 0xE000 - 0x1000 - 0x1000)// protected data (1 block)
    #define PROTECTED_FLASH_HUGE_SZ 0x1000 // 4k  不共基的块大小
#endif


/**
 * @brief flash 设置保护,enable 设置ture为全保护，false为半保护
 *
 * @return OPRT_OK
 */
int tkl_flash_set_protect(const bool enable)
{
    DD_HANDLE flash_handle;
    unsigned int  param;
    unsigned int status;

    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    if (enable) {
        param = FLASH_PROTECT_ALL;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    } else {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }

    ddev_close(flash_handle);
    return OPRT_OK;
}


/**
* @brief read flash
*
* @param[in] addr: flash address
* @param[out] dst: pointer of buffer
* @param[in] size: size of buffer
*
* @note This API is used for reading flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_read(UINT_T addr, UCHAR_T *dst, UINT_T size)
{
    unsigned int status;

    if (NULL == dst) {
        return OPRT_INVALID_PARM;
    }

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_lock();

    DD_HANDLE flash_handle;
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_read(flash_handle, (char *)dst, size, addr);
    ddev_close(flash_handle);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_unlock();

    return OPRT_OK;
}

static unsigned int __uni_flash_is_protect_all(void)
{
    DD_HANDLE flash_handle;
    unsigned int status;
    unsigned int param;

    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    ddev_control(flash_handle, CMD_FLASH_GET_PROTECT, (void *)&param);
    ddev_close(flash_handle);

    return (FLASH_PROTECT_ALL == param);
}

/**
* @brief write flash
*
* @param[in] addr: flash address
* @param[in] src: pointer of buffer
* @param[in] size: size of buffer
*
* @note This API is used for writing flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_write(UINT_T addr, CONST UCHAR_T *src, UINT_T size)
{
    DD_HANDLE flash_handle;
    unsigned int protect_flag;
    unsigned int status;
    unsigned int param;

    if (NULL == src) {
        return OPRT_INVALID_PARM;
    }

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_lock();

    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);

    //解保护
    protect_flag = __uni_flash_is_protect_all();
    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);
    
    if (protect_flag) {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }

    ddev_write(flash_handle, (char *)src, size, addr);

    ddev_close(flash_handle);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_unlock();
    return OPRT_OK;
}

/**
* @brief erase flash
*
* @param[in] addr: flash address
* @param[in] size: size of flash block
*
* @note This API is used for erasing flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_erase(UINT_T addr, UINT_T size)
{
    unsigned short start_sec = (addr / PARTITION_SIZE);
    unsigned short end_sec = ((addr + size - 1) / PARTITION_SIZE);
    unsigned int status;
    unsigned int i = 0;
    unsigned int sector_addr;
    DD_HANDLE flash_handle;
    unsigned int  param;
    unsigned int protect_flag;

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_lock();

    flash_handle = ddev_open(FLASH_DEV_NAME, &status, 0);

    //解保护
    protect_flag = __uni_flash_is_protect_all();
    if (protect_flag) {
        param = FLASH_PROTECT_HALF;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }

    for (i = start_sec; i <= end_sec; i++) {
        sector_addr = PARTITION_SIZE * i;
        ddev_control(flash_handle, CMD_FLASH_ERASE_SECTOR, (void *)(&sector_addr));
    }

    protect_flag = __uni_flash_is_protect_all();    
    if(protect_flag)
    {
        param = FLASH_PROTECT_ALL;
        ddev_control(flash_handle, CMD_FLASH_SET_PROTECT, (void *)&param);
    }

    ddev_close(flash_handle);

    /* TODO: need to consider whether to use locks at the TKL layer*/
    hal_flash_unlock();

    return OPRT_OK;
}

/**
* @brief lock flash
*
* @param[in] addr: lock begin addr
* @param[in] size: lock area size
*
* @note This API is used for lock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_lock(UINT_T addr, UINT_T size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief unlock flash
*
* @param[in] addr: unlock begin addr
* @param[in] size: unlock area size
*
* @note This API is used for unlock flash.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_unlock(UINT_T addr, UINT_T size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
* @brief get flash information
*
* @param[out] info: the description of the flash
*
* @note This API is used to get description of storage.
*
* @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
*/
OPERATE_RET tkl_flash_get_one_type_info(TUYA_FLASH_TYPE_E type, TUYA_FLASH_BASE_INFO_T* info)
{
    if ((type > TUYA_FLASH_TYPE_MAX) || (info == NULL)) {
        return OPRT_INVALID_PARM;
    }
    switch (type) {
        case TUYA_FLASH_TYPE_UF:
            info->partition_num = 1;
            info->partition[0].block_size =  PARTITION_SIZE;
            info->partition[0].start_addr = UF_PARTITION_START;
            info->partition[0].size = UF_PARTITION_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_DATA:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_START;
            info->partition[0].size = SIMPLE_FLASH_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_SWAP:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_SWAP_START;
            info->partition[0].size = SIMPLE_FLASH_SWAP_SIZE;
            break;
       case TUYA_FLASH_TYPE_KV_KEY:
            info->partition_num = 1;
            info->partition[0].block_size = FLH_BLOCK_SZ;
            info->partition[0].start_addr = SIMPLE_FLASH_KEY_ADDR;
            info->partition[0].size = FLH_BLOCK_SZ;
            break;
#if defined(KV_PROTECTED_ENABLE) && (KV_PROTECTED_ENABLE==1)
        case TUYA_FLASH_TYPE_KV_PROTECT:
            info->partition_num = 1;
            info->partition[0].block_size = PROTECTED_FLASH_HUGE_SZ;
            info->partition[0].start_addr = PROTECTED_DATA_ADDR;
            info->partition[0].size = PROTECTED_FLASH_HUGE_SZ;
            break;
#endif
        default:
            return OPRT_INVALID_PARM;
            break;
    }

    return OPRT_OK;

}

