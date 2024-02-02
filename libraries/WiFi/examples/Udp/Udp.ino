#include <WiFi.h>
#include <WiFiUdp.h>

#define WIFI_SSID       "your-ssid"
#define WIFI_PASSWORD   "your-password"

unsigned int localPort = 8888;  // local port to listen on

#define UDP_TX_MAX_SIZE 1024

// buffers for receiving and sending data
char packetBuffer[UDP_TX_MAX_SIZE + 1];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged\r\n";        // a string to send back

WiFiUdp Udp;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (!WiFi.isConnected()) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("UDP server on port ");
  Serial.println(localPort);

  Udp.begin(localPort);
}

void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.print("packet size: ");
    Serial.println(packetSize);
    Serial.print("remoteIP: ");
    Serial.println(Udp.remoteIP().toString());
    Serial.print("remotePort: ");
    Serial.println(Udp.remotePort());


    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, UDP_TX_MAX_SIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write((const uint8_t*)ReplyBuffer, 14);
    Udp.endPacket();
  }
  
  delay(1);
}
