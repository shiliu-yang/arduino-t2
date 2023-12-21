#include <WiFi.h>

#define AP_SSID       "tuya-t2"
#define AP_PASSWORD   "12345678"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.beginAP(AP_SSID, AP_PASSWORD);
}

void loop() {
  // put your main code here, to run repeatedly:

  delay(1000);
}
