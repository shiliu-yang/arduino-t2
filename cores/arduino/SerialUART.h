#ifndef __SERIAL_UART_H__
#define __SERIAL_UART_H__

#include "tkl_uart.h"
#include "tal_mutex.h"

#include "api/RingBuffer.h"
#include "Arduino.h"
#include "api/HardwareSerial.h"

namespace arduino {

class SerialUART : public HardwareSerial
{
public:
    SerialUART(TUYA_UART_NUM_E id);
    ~SerialUART();

    void begin(unsigned long);
    void begin(unsigned long baudrate, uint16_t config);
    void end();
    int available(void);
    int peek(void);
    int read(void);
    void flush(void);
    size_t write(uint8_t);
    size_t write(const uint8_t*, size_t);
    using Print::write; // pull in write(str) and write(buf, size) from Print
    operator bool();

    // rx callback function
    int __rxBufWrite(uint8_t c);
private:
    TUYA_UART_NUM_E __uartID = UART_NUM_MAX;
    MUTEX_HANDLE __mutex;
    RingBufferN<256> _rxBuffer;
};

}

// extern arduino::SerialUART _SerialUART0_;
extern arduino::SerialUART _SerialUART1_;

#endif // __SERIAL_UART_H__
