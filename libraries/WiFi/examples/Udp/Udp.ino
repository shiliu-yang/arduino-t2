#include <WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

unsigned int localPort = 8888;  // local port to listen on

#define UDP_TX_PACKET_MAX_SIZE 1024

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE + 1];  // buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged\r\n";        // a string to send back

WiFiUdp Udp;

void setup() {
  Serial.begin(115200);
  WiFi.begin(STASSID, STAPSK);
  while (!WiFi.isConnected()) {
    Serial.print('.');
    delay(500);
  }
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
    int n = Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);
  }
  
  delay(100);
}
