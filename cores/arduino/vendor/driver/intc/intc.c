/**
 ****************************************************************************************
 *
 * @file intc.c
 *
 * @brief Definition of the Interrupt Controller (INTCTRL) API.
 *
 * Copyright (C) RivieraWaves 2011-2016
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "compiler.h"
#include "intc.h"
#include "intc_pub.h"
#include "include.h"
#include "arm_arch.h"
#include "drv_model_pub.h"
#include "icu_pub.h"
#include "mem_pub.h"
#include "uart_pub.h"

#if CFG_SUPPORT_ALIOS
#include "ll.h"

extern void do_irq( void );
extern void do_fiq( void );
extern void do_swi( void );
#else
#include "power_save_pub.h"
#endif
#include "start_type_pub.h"

ISR_T _isrs[INTC_MAX_COUNT] = {{{0, 0}},};
static UINT32 isrs_mask = 0;
static ISR_LIST_T isr_hdr = {{&isr_hdr.isr, &isr_hdr.isr},};
IRDA_CHECK_FUNC func_irda_check = NULL;

void intc_register_irda_check_func(IRDA_CHECK_FUNC func)
{
	func_irda_check = func;
}

void intc_unregister_irda_check_func()
{
	func_irda_check = NULL;
}

void intc_hdl_entry(UINT32 int_status)
{
    UINT32 i;
    ISR_T *f;
    UINT32 status;
    LIST_HEADER_T *n;
    LIST_HEADER_T *pos;

    status = int_status & isrs_mask;
    INTC_PRT("intc:%x:%x\r\n", int_status, status);

    #if CFG_USE_STA_PS
    power_save_dtim_wake(status);
    #endif

    list_for_each_safe(pos, n, &isr_hdr.isr)
    {
        f = list_entry(pos, ISR_T, list);
        i = f->int_num;

        if ((BIT(i) & status))
        {
            f->isr_func();
            status &= ~(BIT(i));
        }

        if(0 == status)
        {
            return;
        }
    }
}

void intc_service_register(UINT8 int_num, UINT8 int_pri, FUNCPTR isr)
{
    LIST_HEADER_T *pos, *n;
    ISR_T *tmp_ptr, *cur_ptr;
    ISR_T buf_ele;

    GLOBAL_INT_DECLARATION();

    buf_ele           = _isrs[int_num];
    cur_ptr           = &_isrs[int_num];
    cur_ptr->isr_func = isr;
    cur_ptr->int_num  = int_num;
    cur_ptr->pri      = int_pri;

    INTC_PRT("reg_isr:%d:%d:%p\r\n", int_num, int_pri, isr);

    GLOBAL_INT_DISABLE();
    if (list_empty(&isr_hdr.isr))
    {
        list_add_head(&cur_ptr->list, &isr_hdr.isr);
        goto ok;
    }

    /* Insert the ISR to the function list, this list is sorted by priority number */
    list_for_each_safe(pos, n, &isr_hdr.isr)
    {
        tmp_ptr = list_entry(pos, ISR_T, list);

        if (int_pri < tmp_ptr->pri)
        {
            /* add entry at the head of the queue */
            list_add_tail(&cur_ptr->list, &tmp_ptr->list);

            INTC_PRT("reg_isr_o1\r\n");

            goto ok;
        }
        else if (int_pri == tmp_ptr->pri)
        {
            INTC_PRT("reg_isr_error\r\n");
            goto error;
        }
    }

    list_add_tail(&cur_ptr->list, &isr_hdr.isr);
    INTC_PRT("reg_isr_o2\r\n");

ok:
    isrs_mask |= BIT(int_num);
    GLOBAL_INT_RESTORE();

    return;
error:
    /* something wrong  */
    _isrs[int_num] = buf_ele;
    GLOBAL_INT_RESTORE();

    return;
}

void intc_service_change_handler(UINT8 int_num, FUNCPTR isr)
{
    LIST_HEADER_T *pos, *n;
    ISR_T *tmp_ptr, *cur_ptr;
    ISR_T buf_ele;
    UINT8 int_pri;

    GLOBAL_INT_DECLARATION();

    buf_ele           = _isrs[int_num];
    cur_ptr           = &_isrs[int_num];
    int_pri = cur_ptr->pri;

    if(!cur_ptr->isr_func)
        return;

    INTC_PRT("reg_isr:%d:%d:%p\r\n", int_num, int_pri, isr);

    GLOBAL_INT_DISABLE();
    if (list_empty(&isr_hdr.isr))
    {
        goto exit;
    }

    /* Insert the ISR to the function list, this list is sorted by priority number */
    list_for_each_safe(pos, n, &isr_hdr.isr)
    {
        tmp_ptr = list_entry(pos, ISR_T, list);

        if (int_pri == tmp_ptr->pri)
        {
            buf_ele.isr_func = isr;
            break;
        }
    }

exit:
    /* something wrong  */
    _isrs[int_num] = buf_ele;
    GLOBAL_INT_RESTORE();

    return;
}

/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */
void intc_spurious(void)
{
    ASSERT(0);
}

void intc_enable(int index)
{
    UINT32 param;

    param = (1UL << index);
    sddev_control(ICU_DEV_NAME, CMD_ICU_INT_ENABLE, &param);
}

void intc_disable(int index)
{
    UINT32 param;

    param = (1UL << index);
    sddev_control(ICU_DEV_NAME, CMD_ICU_INT_DISABLE, &param);
}

void rf_ps_wakeup_isr_idle_int_cb()
{
#if ( CONFIG_APP_MP3PLAYER == 1 )
    UINT32 irq_status;

    irq_status = sddev_control(ICU_DEV_NAME, CMD_GET_INTR_STATUS, 0);

    if(irq_status & 1<<IRQ_I2S_PCM)
    {
    irq_status &= 1<<IRQ_I2S_PCM;
    i2s_isr();
    sddev_control(ICU_DEV_NAME, CMD_CLR_INTR_STATUS, &irq_status);
    }
#endif    
}

void intc_irq(void)
{
    UINT32 irq_status;
	
    irq_status = icu_ctrl(CMD_GET_INTR_STATUS, 0);
    irq_status = irq_status & 0xFFFF;
	if(0 == irq_status)
	{
	    #if (! CFG_USE_STA_PS)
		os_printf("irq:dead\r\n");
        #endif
	}
	
    icu_ctrl(CMD_CLR_INTR_STATUS, &irq_status);

    intc_hdl_entry(irq_status);
}

void intc_fiq(void)
{
    UINT32 fiq_status;

	/* bk_ir_check_timer*/
    if (func_irda_check && ((*func_irda_check)() == 0))
    {
        return;
    }

    fiq_status = icu_ctrl(CMD_GET_INTR_STATUS, 0);
    fiq_status = fiq_status & 0xFFFF0000;
    icu_ctrl(CMD_CLR_INTR_STATUS, &fiq_status);

    intc_hdl_entry(fiq_status);
}

#if CFG_SUPPORT_ALIOS
void deafult_swi(void)
{
    while(1);
}
#endif

void intc_init(void)
{
    UINT32 param;

    *((volatile uint32_t *)0x400000) = (uint32_t)&do_irq;
    *((volatile uint32_t *)0x400004) = (uint32_t)&do_fiq;
    *((volatile uint32_t *)0x400008) = (uint32_t)&do_swi;
    *((volatile uint32_t *)0x40000c) = (uint32_t)&do_undefined;
    *((volatile uint32_t *)0x400010) = (uint32_t)&do_pabort;
    *((volatile uint32_t *)0x400014) = (uint32_t)&do_dabort;
    *((volatile uint32_t *)0x400018) = (uint32_t)&do_reserved;

    intc_enable(FIQ_MAC_GENERAL);
    intc_enable(FIQ_MAC_PROT_TRIGGER);

    intc_enable(FIQ_MAC_TX_TRIGGER);
    intc_enable(FIQ_MAC_RX_TRIGGER);

    intc_enable(FIQ_MAC_TX_RX_MISC);
    intc_enable(FIQ_MAC_TX_RX_TIMER);
	
    intc_enable(FIQ_MODEM);	

    param = GINTR_FIQ_BIT | GINTR_IRQ_BIT;
    sddev_control(ICU_DEV_NAME, CMD_ICU_GLOBAL_INT_ENABLE, &param);

    return;
}

void intc_deinit(void)
{
    UINT32 param;
	
    for( int i = 0; i<=FIQ_DPLL_UNLOCK; i++)
	{
        intc_disable(i);
	}
	
    param = GINTR_FIQ_BIT | GINTR_IRQ_BIT;
    sddev_control(ICU_DEV_NAME, CMD_ICU_GLOBAL_INT_DISABLE, &param);

    return;
}

void bk_cpu_shutdown(void)
{
    GLOBAL_INT_DECLARATION();
	
    os_printf("shutdown...\n");
	
    GLOBAL_INT_DISABLE();	
    while(1);
    GLOBAL_INT_RESTORE();
}

void bk_show_register (struct arm_registers *regs)
{
    os_printf("Current regs:\n");
    os_printf("r0:0x%08x r1:0x%08x r2:0x%08x r3:0x%08x\n",
               regs->r0, regs->r1, regs->r2, regs->r3);
    os_printf("r4:0x%08x r5:0x%08x r6:0x%08x r7:0x%08x\n",
               regs->r4, regs->r5, regs->r6, regs->r7);
    os_printf("r8:0x%08x sb:0x%08x sl:0x%08x fp:0x%08x\n",
               regs->r8, regs->r9, regs->r10, regs->fp);
    os_printf("ip:0x%08x sp:0x%08x lr:0x%08x pc:0x%08x\n",
               regs->ip, regs->sp, regs->lr, regs->pc);
    os_printf("SPSR:0x%08x\n", regs->spsr);
    os_printf("CPSR:0x%08x\n", regs->cpsr);

    int i;
    const unsigned int *reg1;

    os_printf("\nseparate regs:\n");

    reg1 = (const unsigned int *)0x400024;
    os_printf("SYS:cpsr r8-r14\n");
    for(i=0;i<0x20>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }

    os_printf("%s:cpsr spsr r8-r14\n", "IRQ");
    reg1 = (const unsigned int *)0x400044;
    for(i=0;i<0x24>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }

    os_printf("%s:cpsr spsr r8-r14\n", "FIR");
    reg1 = (const unsigned int *)0x400068;
    for(i=0;i<0x24>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }

    os_printf("%s:cpsr spsr r8-r14\n", "ABT");
    reg1 = (const unsigned int *)0x40008c;
    for(i=0;i<0x24>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }

    os_printf("%s:cpsr spsr r8-r14\n", "UND");
    reg1 = (const unsigned int *)0x4000b0;
    for(i=0;i<0x24>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }

    os_printf("%s:cpsr spsr r8-r14\n", "SVC");
    reg1 = (const unsigned int *)0x4000d4;
    for(i=0;i<0x24>>2;i++)
    {
        os_printf("0x%08x\n",*(reg1 + i));
    }
    
    os_printf("\r\n");

}

void bk_trap_udef(struct arm_registers *regs)
{
#if (CFG_SOC_NAME == SOC_BK7231N)
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)(CRASH_UNDEFINED_VALUE & 0xffff);
#else
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_UNDEFINED_VALUE;
#endif
    os_printf("undef instruction\n");
    bk_show_register(regs);
    bk_cpu_shutdown();
}

void bk_trap_pabt(struct arm_registers *regs)
{
#if (CFG_SOC_NAME == SOC_BK7231N)
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)(CRASH_PREFETCH_ABORT_VALUE & 0xffff);
#else
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_PREFETCH_ABORT_VALUE;
#endif
    os_printf("prefetch abort\n");
    bk_show_register(regs);
    bk_cpu_shutdown();
}

void bk_trap_dabt(struct arm_registers *regs)
{
#if (CFG_SOC_NAME == SOC_BK7231N)
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)(CRASH_DATA_ABORT_VALUE & 0xffff);
#else
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_DATA_ABORT_VALUE;
#endif
    os_printf("data abort\n");
    bk_show_register(regs);
    bk_cpu_shutdown();
}

void bk_trap_resv(struct arm_registers *regs)
{
#if (CFG_SOC_NAME == SOC_BK7231N)
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)(CRASH_UNUSED_VALUE & 0xffff);
#else
    *((volatile uint32_t *)START_TYPE_ADDR) = (uint32_t)CRASH_UNUSED_VALUE;
#endif
    os_printf("not used\n");
    bk_show_register(regs);
    bk_cpu_shutdown();
}

/// @}
