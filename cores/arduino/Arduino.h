#ifndef ARDUINO_H
#define ARDUINO_H

#include "pinMap.h"
#include "api/ArduinoAPI.h"

#if defined(__cplusplus) && !defined(c_plusplus)

// C functions defined
#if (defined(__cplusplus)||defined(c_plusplus))
extern "C"{
#endif

void tuya_app_main(void);

#if (defined(__cplusplus)||defined(c_plusplus))
}
#endif

using namespace arduino;

#include "SerialUART.h"

// #define Serial _SerialUART0_

// #define Serial1 _SerialUART0_
#define Serial2 _SerialUART1_

#endif

#endif // ARDUINO_H
