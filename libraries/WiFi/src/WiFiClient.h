/*
  Client.h - Base class that provides Client
  Copyright (c) 2011 Adrian McEwen.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <Arduino.h>
#include "api/Client.h"
#include "cbuf.h"

#define TCP_TX_PACKET_MAX_SIZE 1436
#define TCP_RX_PACKET_MAX_SIZE 1436

class WiFiClient : public Client {
private:
  uint32_t _timeoutMs;
  int _sockfd;

  cbuf *_rxBuff;

  bool _connected;
public:
  WiFiClient();
  ~WiFiClient();
  int connect(IPAddress ip, uint16_t port);
  int connect(IPAddress ip, uint16_t port, uint32_t timeoutMs);
  int connect(const char *host, uint16_t port);
  size_t write(uint8_t data);
  size_t write(const uint8_t *buf, size_t size);
  using Print::write;

  int available();
  int read();
  int read(uint8_t *buf, size_t size);
  int peek();
  void flush();
  void stop();
  uint8_t connected();

  operator bool()
  {
      return connected();
  }
protected:
  // uint8_t* rawIPAddress(IPAddress& addr) { return addr.raw_address(); };
};
