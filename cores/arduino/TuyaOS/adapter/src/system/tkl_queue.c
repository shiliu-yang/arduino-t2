/**
 * @file tkl_queue.c
 * @brief the default weak implements of tuya os queue, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tkl_queue.h"
#include "tkl_system.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

typedef struct {
    xQueueHandle queue;
} QUEUE_MANAGE, *P_QUEUE_MANAGE;

/**
 * @brief Create message queue
 *
 * @param[in] msgsize message size
 * @param[in] msgcount message number
 * @param[out] queue the queue handle created
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_create_init(TKL_QUEUE_HANDLE *queue, INT_T msgsize, INT_T msgcount)
{
    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    *queue = NULL;

    *queue = (TKL_QUEUE_HANDLE)xQueueCreate(msgcount, msgsize);
    if (*queue == NULL) {
        return OPRT_OS_ADAPTER_QUEUE_CREAT_FAILED;
    }

	return OPRT_OK;
}

/**
 * @brief post a message to the message queue
 *
 * @param[in] queue the handle of the queue
 * @param[in] data the data of the message
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_post(CONST TKL_QUEUE_HANDLE queue, VOID_T *data, UINT_T timeout)
{
    int ret = pdPASS;
	
    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    extern uint32_t bk_wlan_get_INT_status(void);
    if (FALSE == bk_wlan_get_INT_status()) {
        if (timeout == TKL_QUEUE_WAIT_FROEVER) {
            ret = xQueueSend(queue, data, portMAX_DELAY);
        } else {
            UINT_T ticks = timeout / portTICK_RATE_MS;

            if (ticks == 0) {
                ticks = 1;
            }
            ret = xQueueSend(queue, data, ticks);
        }
    } else {
        signed portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
        ret = xQueueSendFromISR(queue, data, &xHigherPriorityTaskWoken);
        portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
    }

    if (pdPASS != ret) {
        return OPRT_OS_ADAPTER_QUEUE_SEND_FAIL;
    }

	return OPRT_OK;
}

/**
 * @brief fetch message from the message queue
 *
 * @param[in] queue the message queue handle
 * @param[inout] msg the message fetch form the message queue
 * @param[in] timeout timeout time
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_queue_fetch(CONST TKL_QUEUE_HANDLE queue, VOID_T *msg, UINT_T timeout)
{
    void *dummyptr;
    int ret = pdPASS;

    if (!queue) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (msg == NULL) {
        msg = &dummyptr;
    }

    if (timeout == TKL_QUEUE_WAIT_FROEVER) {
        ret = xQueueReceive(queue, msg, portMAX_DELAY);
    } else {
        UINT_T ticks = timeout / portTICK_RATE_MS;

        if (ticks == 0) {
            ticks = 1;
        }
        ret = xQueueReceive(queue, msg, ticks);
    }
								
    if (pdPASS != ret) {
        return OPRT_OS_ADAPTER_QUEUE_SEND_FAIL;
    }

	return OPRT_OK;
}


/**
 * @brief free the message queue
 *
 * @param[in] queue the message queue handle
 *
 * @return VOID_T
 */
VOID_T tkl_queue_free(CONST TKL_QUEUE_HANDLE queue)
{
    if (!queue) {
        return ;
    }

    if (uxQueueMessagesWaiting((QueueHandle_t)queue)) {
        /* Line for breakpoint.  Should never break here! */
        portNOP();
        // TODO notify the user of failure.
    }

    vQueueDelete((QueueHandle_t)queue);
}

