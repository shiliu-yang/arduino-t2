#include "WiFiClient.h"

extern "C" {
#include "tal_memory.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"
#include "tal_network.h"
#include "stdint.h"
}//extern "C"

extern "C" void bk_printf(const char *fmt, ...);

#define DEBUG_PRINTF bk_printf

#define WIFI_CLIENT_MAX_WRITE_RETRY      (10)
#define WIFI_CLIENT_SELECT_TIMEOUT_US    (1000000)

#define RECV_TMP_BUFF_SIZE              (125)

#undef connect
#undef write
#undef read

WiFiClient::WiFiClient()
: _timeoutMs(50)
, _sockfd(-1)
, _rxBuff(nullptr)
{

}

WiFiClient::~WiFiClient()
{
  stop();
}

int WiFiClient::connect(IPAddress ip, uint16_t port)
{
  return connect(ip, port, _timeoutMs);
}

int WiFiClient::connect(IPAddress ip, uint16_t port, uint32_t timeoutMs)
{
  _timeoutMs = timeoutMs;
  int sockfd = tal_net_socket_create(PROTOCOL_TCP);
  if (sockfd < 0) {
    DEBUG_PRINTF("socket: %d", errno);
    return 0;
  }
  tal_net_set_block(sockfd, 0);

  struct timeval tv;
  tv.tv_sec = _timeoutMs / 1000;
  tv.tv_usec = (_timeoutMs  % 1000) * 1000;

  uint32_t tmpIP = static_cast<uint32_t>(ip);
  TUYA_IP_ADDR_T serverIP = (TUYA_IP_ADDR_T)UNI_HTONL(tmpIP);
  TUYA_ERRNO res = tal_net_connect(sockfd, serverIP, port);

  if (res != 0 &&  errno != EINPROGRESS) {
    DEBUG_PRINTF("connect on fd %d, res: %d, \"%s\"", sockfd, res, strerror(errno));
    tal_net_close(sockfd);
    return 0;
  }

  TUYA_FD_SET_T fdset;
  TAL_FD_ZERO(&fdset);
  TAL_FD_SET(sockfd, &fdset);
  tal_net_select(sockfd + 1, NULL, &fdset, NULL, _timeoutMs<0 ? 50 : (UINT_T)_timeoutMs);
  if (res < 0) {
    DEBUG_PRINTF("select on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
    tal_net_close(sockfd);
    return 0;
  } else if (res == 0) {
    DEBUG_PRINTF("select returned due to timeout %d ms for fd %d, \"%s\"\r\n", _timeoutMs, sockfd, strerror(errno));
    tal_net_close(sockfd);
    return 0;
  } else {
    int sockerr;
    INT_T len = (INT_T)sizeof(int);
    res = tal_net_getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);

    if (res < 0) {
      DEBUG_PRINTF("getsockopt on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
      tal_net_close(sockfd);
      return 0;
    }

    if (sockerr != 0) {
      DEBUG_PRINTF("socket error on fd %d, errno: %d, \"%s\"", sockfd, sockerr, strerror(sockerr));
      tal_net_close(sockfd);
      return 0;
    }
  }

#define ROE_WIFICLIENT(x,msg) { if (((x)<0)) {DEBUG_PRINTF("Setsockopt '" msg "'' on fd %d failed. errno: %d, \"%s\"", sockfd, errno, strerror(errno)); return 0; }}
  ROE_WIFICLIENT(tal_net_setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)),"SO_SNDTIMEO");
  ROE_WIFICLIENT(tal_net_setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)),"SO_RCVTIMEO");

  tal_net_set_block(sockfd, 1);
  _sockfd = sockfd;
  if (_rxBuff) {
    _rxBuff->flush();
  }

  _connected = true;
  return 1;
}

int WiFiClient::connect(const char *host, uint16_t port)
{
  return 1;
}

size_t WiFiClient::write(uint8_t data)
{
  return write(&data, 1);
}

size_t WiFiClient::write(const uint8_t *buf, size_t size)
{
  int res =0;
  int retry = WIFI_CLIENT_MAX_WRITE_RETRY;
  int socketFD = _sockfd;
  size_t totalBytesSent = 0;
  size_t bytesRemaining = size;

  if(!_connected || (socketFD < 0)) {
    return 0;
  }

  while(retry) {
    TUYA_FD_SET_T set;
    TAL_FD_ZERO(&set);        // empties the set
    TAL_FD_SET(socketFD, &set); // adds FD to the set
    UINT_T timeoutMs = WIFI_CLIENT_SELECT_TIMEOUT_US / 1000;
    retry--;

    if(tal_net_select(socketFD + 1, NULL, &set, NULL, timeoutMs) < 0) {
      return 0;
    }

    if(TAL_FD_ISSET(socketFD, &set)) {
      res = tal_net_send(socketFD, (void*) buf, bytesRemaining);
      if(res > 0) {
        totalBytesSent += res;
        if (totalBytesSent >= size) {
          //completed successfully
          retry = 0;
        } else {
          buf += res;
          bytesRemaining -= res;
          retry = WIFI_CLIENT_MAX_WRITE_RETRY;
        }
      } else if(res < 0) {
        if(errno != EAGAIN) {
          this->stop();
          res = 0;
          retry = 0;
        }
      } else {
        // Try again
      }
    }
  }

  return totalBytesSent;
}

int WiFiClient::available()
{
  read(nullptr, 0);
  return (int)_rxBuff->available();
}

int WiFiClient::read()
{
  uint8_t readVal;
  read(&readVal, 1);

  return (int)readVal;
}

int WiFiClient::read(uint8_t *buf, size_t size)
{
  uint8_t *tmpBuff = NULL;

  if (-1 == _sockfd) {
    return 0;
  }

  int res = 0;

  if (nullptr == _rxBuff) {
    _rxBuff = new cbuf(TCP_RX_PACKET_MAX_SIZE);
    if (nullptr == _rxBuff) {
      return -1;
    }
  }

  TUYA_FD_SET_T readfds;
  TAL_FD_ZERO(&readfds);
  TAL_FD_SET(_sockfd, &readfds);
  res = tal_net_select(_sockfd + 1, &readfds, NULL, NULL, 1);
  if (res > 0 && TAL_FD_ISSET(_sockfd, &readfds)) {
    tmpBuff = (uint8_t *)tal_malloc(RECV_TMP_BUFF_SIZE);
    if (NULL == tmpBuff) {
      return -1;
    }
    while (1) {
      res = tal_net_recv(_sockfd, tmpBuff, RECV_TMP_BUFF_SIZE);
      if (res > 0) {
        _rxBuff->write((const char*)tmpBuff, (size_t)res);
      } else {
        break;
      }
    }
    tal_free(tmpBuff);
    tmpBuff = NULL;
  }

  if (nullptr != buf && size > 0) {
    _rxBuff->read((char *)buf, size);
  }

  return res;
}

int WiFiClient::peek()
{
  read(nullptr, 0);
  return _rxBuff->peek();
}

void WiFiClient::flush()
{
  while (available()) {
    _rxBuff->flush();
  }
}

void WiFiClient::stop()
{
  tal_net_close(_sockfd);
  _sockfd = -1;
  if (_rxBuff) {
    cbuf *b = _rxBuff;
    _rxBuff = nullptr;
    delete b;
  }
}

uint8_t WiFiClient::connected()
{
  if (_connected) {
    uint8_t dummy;
    int res = recv(_sockfd, &dummy, 0, MSG_DONTWAIT);
    // avoid unused var warning by gcc
    (void)res;
    // recv only sets errno if res is <= 0
    if (res <= 0){
      switch (errno) {
        case EWOULDBLOCK:
        case ENOENT: //caused by vfs
          _connected = true;
          break;
        case ENOTCONN:
        case EPIPE:
        case ECONNRESET:
        case ECONNREFUSED:
        case ECONNABORTED:
          _connected = false;
          DEBUG_PRINTF("Disconnected: RES: %d, ERR: %d", res, errno);
          break;
        default:
          DEBUG_PRINTF("Unexpected: RES: %d, ERR: %d", res, errno);
          _connected = true;
          break;
      }
    } else {
      _connected = true;
    }
  }
  return _connected;
}
