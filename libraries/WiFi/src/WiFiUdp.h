#pragma once

#include <Arduino.h>
#include "cbuf.h"

#define UDP_TX_PACKET_MAX_SIZE 1460
#define UDP_RX_PACKET_MAX_SIZE 1460

class WiFiUdp : public UDP{
private:
  int _udpSocket;
  uint16_t _port;

  IPAddress _remoteIP;
  uint16_t _remotePort;
  cbuf *_rxBuff;

public:
  WiFiUdp();
  ~WiFiUdp();

  uint8_t begin(uint16_t);  // initialize, start listening on specified port. Returns 1 if successful, 0 if there are no sockets available to use
  uint8_t begin(IPAddress a, uint16_t p);
  uint8_t beginMulticast(IPAddress, uint16_t);  // initialize, start listening on specified multicast IP address and port. Returns 1 if successful, 0 on failure
  void stop();  // Finish with the UDP socket

  // Sending UDP packets
  
  // Start building up a packet to send to the remote host specified in ip and port
  // Returns 1 if successful, 0 if there was a problem with the supplied IP address or port
  int beginPacket(IPAddress ip, uint16_t port);
  // Start building up a packet to send to the remote host specified in host and port
  // Returns 1 if successful, 0 if there was a problem resolving the hostname or port
  int beginPacket(const char *host, uint16_t port);
  // Finish off this packet and send it
  // Returns 1 if the packet was sent successfully, 0 if there was an error
  int endPacket();
  // Write a single byte into the packet
  size_t write(uint8_t);
  // Write size bytes from buffer into the packet
  size_t write(const uint8_t *buffer, size_t size);

  // Start processing the next available incoming packet
  // Returns the size of the packet in bytes, or 0 if no packets are available
  int parsePacket();
  // Number of bytes remaining in the current packet
  int available();
  // Read a single byte from the current packet
  int read();
  // Read up to len bytes from the current packet and place them into buffer
  // Returns the number of bytes read, or 0 if none are available
  int read(unsigned char* buffer, size_t len);
  // Read up to len characters from the current packet and place them into buffer
  // Returns the number of characters read, or 0 if none are available
  int read(char* buffer, size_t len);
  // Return the next byte from the current packet without moving on to the next byte
  int peek();
  void flush();	// Finish reading the current packet

  // Return the IP address of the host who sent the current incoming packet
  IPAddress remoteIP();
  // Return the port of the host who sent the current incoming packet
  uint16_t remotePort();

};


