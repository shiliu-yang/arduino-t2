#include <WiFi.h>

#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (WiFi.isConnected()) {
    Serial.println("WiFi connected");
  } else {
    Serial.println("WiFi not connected");
  }

  delay(1000);
}
