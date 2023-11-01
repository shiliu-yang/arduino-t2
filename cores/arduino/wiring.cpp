#include "tal_system.h"

#include "FreeRTOS.h"
#include "portmacro.h"

#ifdef __cplusplus
extern "C"{
#endif

unsigned long millis()
{
    unsigned long ms = tal_system_get_millisecond();
    return ms;
}

void delay(unsigned long ms)
{
    return tal_system_sleep((UINT_T)ms);
}

void yield(void)
{
    portYIELD();
    return;
}

#ifdef __cplusplus
}
#endif
