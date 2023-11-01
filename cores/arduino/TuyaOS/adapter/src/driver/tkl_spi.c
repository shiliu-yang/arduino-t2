#include "tkl_spi.h"
#include "tkl_gpio.h"
#include "tkl_mutex.h"

#include "drv_model_pub.h"
#include "spi_pub.h"

static TKL_MUTEX_HANDLE spi_mutex = NULL;
static bool cs_auto_flag = true;            //是否由适配层自动控制cs, false由上层自己控制
static bool spic_flag = false;              //特殊模式，幻彩灯带驱动下需置一，可连续发送大于4K以上数据
#define SPI_CS_PIN GPIO_NUM_15


/**
* @如果需要连续发送4K以上数据（不分包），必须调用这个接口
*/
void tkl_spi_set_spic_flag(void)
{
    spic_flag = true;
}

/**
 * @brief spi init
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_BASE_CFG_T *cfg)
{
    if(cfg->role == TUYA_SPI_ROLE_MASTER) {
        uint8_t mode ;
        mode |= (cfg->mode & 0x1) << 2;
        mode |= (cfg->mode & 0x2) << 1;
        bk_spi_master_dma_init(mode, cfg->freq_hz, spic_flag);
        // 若cs_pin 为15号脚，pin脚初始化需在spi初始化之后，否则该管脚无法控制
        cs_auto_flag = (cfg->type == TUYA_SPI_AUTO_TYPE) ? true : false;
        if(cs_auto_flag) {
            TUYA_GPIO_BASE_CFG_T gpio_cfg = {
                .mode = TUYA_GPIO_PUSH_PULL,
                .direct = TUYA_GPIO_OUTPUT,
                .level = TUYA_GPIO_LEVEL_HIGH
            };
            if(tkl_gpio_init(SPI_CS_PIN, &gpio_cfg)) {
                tkl_log_output("spi cs pin init error\r\n");
                return OPRT_COM_ERROR;
            }
        }

        if(spi_mutex == NULL) {
            if(tkl_mutex_create_init(&spi_mutex)) {
                tkl_log_output("spi init error\r\n");
                return OPRT_COM_ERROR;
            }
        }
    } else {
        return OPRT_NOT_SUPPORTED;
    }
    return OPRT_OK;
}

/**
 * @brief spi deinit
 * 
 * @param[in] port: spi port
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    tkl_mutex_lock(spi_mutex);
    if(cs_auto_flag)
        tkl_gpio_deinit(SPI_CS_PIN);
    bk_spi_master_deinit();
    tkl_mutex_unlock(spi_mutex);
    tkl_mutex_release(spi_mutex);
    spi_mutex = NULL;
    spic_flag = false;
    return OPRT_OK;
}

/**
 * Spi send
 *
 * @param[in]  port     the spi device
 * @param[in]  data     spi send data
 * @param[in]  size     spi send data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    if(spic_flag) {
        tkl_mutex_lock(spi_mutex);
        struct spi_message msg;
        msg.send_buf = data;
        msg.send_len = size;
        if(cs_auto_flag)
            tkl_gpio_write(SPI_CS_PIN, TUYA_GPIO_LEVEL_LOW);
        int ret = bk_spi_master_dma_send(&msg);
        if(cs_auto_flag)
            tkl_gpio_write(SPI_CS_PIN, TUYA_GPIO_LEVEL_HIGH);
        tkl_mutex_unlock(spi_mutex);
        return ret ? OPRT_COM_ERROR : OPRT_OK;
    } else 
        return tkl_spi_transfer(port, data, NULL, size);
}

/**
 * spi_recv
 *
 * @param[in]   port     the spi device
 * @param[out]  data     spi recv data
 * @param[in]   size     spi recv data size
 *
 * @return  OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, VOID_T *data, UINT16_T size)
{
    return tkl_spi_transfer(port, NULL, data, size);
}

/**
 * @brief spi transfer
 * 
 * @param[in] port: spi port
 * @param[in] send_buf: spi send buf
 * @param[out] send_buf:spi recv buf
 * @param[in] length: spi msg length
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
// 全双工功能仍存在问题，接收滞后发送几个时钟。
OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, VOID_T* send_buf, VOID_T* receive_buf, UINT32_T length)
{
    tkl_mutex_lock(spi_mutex);
    struct spi_message msg;
    msg.recv_len = length;
    msg.recv_buf = (uint8_t *)receive_buf;
    msg.send_buf = (uint8_t *)send_buf;
    msg.send_len = length;
    if(cs_auto_flag)
        tkl_gpio_write(SPI_CS_PIN, TUYA_GPIO_LEVEL_LOW);
    int ret = bk_spi_master_dma_xfer(&msg);
    if(cs_auto_flag)
        tkl_gpio_write(SPI_CS_PIN, TUYA_GPIO_LEVEL_HIGH);
    tkl_mutex_unlock(spi_mutex);
    return ret ? OPRT_COM_ERROR : OPRT_OK;
}

/**
 * @brief adort spi transfer,or spi send, or spi recv
 * 
 * @param[in] port: spi port
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief get spi status.
 * 
 * @param[in] port: spi port
 * @param[out]  TUYA_SPI_STATUS_T,please refer to tuya_cloud_types.h
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status)
{
    return bk_spi_get_status();
}

/**
 * @brief spi irq init
 * NOTE: call this API will not enable interrupt
 * 
 * @param[in] port: spi port, id index starts at 0
 * @param[in] cb:  spi irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, CONST TUYA_SPI_IRQ_CB *cb)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi irq enable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief spi irq disable
 * 
 * @param[in] port: spi port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}


#if 0
#include "tkl_system.h"
#include "tkl_output.h"

#define BUF_SIZE 32
uint8_t send_buf[BUF_SIZE];
uint8_t recv_buf[BUF_SIZE];

void spi_test_thread(void *args)
{
    tkl_system_sleep(5000);
    TUYA_SPI_BASE_CFG_T cfg = {
        .mode = TUYA_SPI_MODE0,
        .freq_hz = 6600000,
        .role = TUYA_SPI_ROLE_MASTER,
        .type = TUYA_SPI_AUTO_TYPE
    };
    tkl_spi_init(0, &cfg);

    for(int i=0;i<sizeof(send_buf);i+=2) {
        send_buf[i] = i >> 8;
        send_buf[i+1] = (uint8_t)i;
    }
        
    int cnt = 5;
    while(cnt --) {
        tkl_spi_send(0, send_buf, BUF_SIZE);
        tkl_spi_recv(0,recv_buf, BUF_SIZE);
        for(int i = 0;i<sizeof(recv_buf);i++) {
            if(i % 16 == 0) {
                os_printf("\r\n%4d:",i);
            }
            os_printf("%02x ",recv_buf[i]);
        }
        tkl_spi_transfer(0, send_buf, NULL, BUF_SIZE);
        tkl_spi_transfer(0, send_buf, recv_buf, BUF_SIZE);
        for(int i = 0;i<sizeof(recv_buf);i++) {
            if(i % 16 == 0) {
                os_printf("\r\n%4d:",i);
            }
            os_printf("%02x ",recv_buf[i]);
        }
        tkl_system_sleep(10);
    }
    while(1)
        tkl_system_sleep(10);
}

void tkl_spi_master_test(void)
{
    tkl_thread_create(&spi_test_thread, "spi_test_thread", 4096, 4, spi_test_thread, NULL);
}
#else
void tkl_spi_master_test(void);
#endif