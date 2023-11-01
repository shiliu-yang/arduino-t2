#ifndef ARDUINO_H
#define ARDUINO_H

#include "pinMap.h"
#include "api/ArduinoAPI.h"

// C functions defined
#if (defined(__cplusplus)||defined(c_plusplus))
extern "C"{
#endif

void tuya_app_main(void);

#if (defined(__cplusplus)||defined(c_plusplus))
}
#endif

#endif // ARDUINO_H
