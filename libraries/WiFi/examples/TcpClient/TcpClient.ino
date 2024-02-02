#include <WiFi.h>
#include <WiFiClient.h>

#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"

// Use WiFiClient class to create TCP connections
WiFiClient client;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (!WiFi.isConnected()) {
    Serial.print('.');
    delay(500);
  }
  delay(1000);

  while (!client.connect(IPAddress(192, 168, 137, 1), 1234)) {
    Serial.println("Connection failed.");
    Serial.println("Waiting 5 seconds before retrying...");
    delay(5000);
  }
  Serial.println("Connected!");
}

void loop()
{
  uint8_t readBuf[125] = {0};

  int readSize = client.available();
  if (readSize) {
    readSize = readSize > 125 ? 125 : readSize;
    client.read(readBuf, readSize);
    Serial.print("readSize: ");
    Serial.println(readSize);
    Serial.print("readBuf: ");
    Serial.println((char*)readBuf);
    client.write(readBuf, readSize);
  }

  delay(10);
}

