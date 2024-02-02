#ifndef ARDUINO_H
#define ARDUINO_H

#include "pinMap.h"
#include "api/ArduinoAPI.h"

#if defined(__cplusplus) && !defined(c_plusplus)

using namespace arduino;

#include "SerialUART.h"

#define Serial _SerialUART0_

#define Serial1 _SerialUART0_
#define Serial2 _SerialUART1_

#endif // __cplusplus

#endif // ARDUINO_H
