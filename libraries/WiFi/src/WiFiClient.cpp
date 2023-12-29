#include "WiFiClient.h"

extern "C" {
#include "tal_memory.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"
}//extern "C"

extern "C" void bk_printf(const char *fmt, ...);

#define DEBUG_PRINTF bk_printf

#define WIFI_CLIENT_MAX_WRITE_RETRY      (10)
#define WIFI_CLIENT_SELECT_TIMEOUT_US    (1000000)

#undef connect
#undef write
#undef read

WiFiClient::WiFiClient()
: _timeoutMs(3000)
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
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    DEBUG_PRINTF("socket: %d", errno);
    return 0;
  }
  fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFL, 0 ) | O_NONBLOCK );

  struct sockaddr_in serveraddr;
  memset((char *) &serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = (in_addr_t)ip;
  serveraddr.sin_port = htons(port);
  fd_set fdset;
  struct timeval tv;
  FD_ZERO(&fdset);
  FD_SET(sockfd, &fdset);
  tv.tv_sec = _timeoutMs / 1000;
  tv.tv_usec = (_timeoutMs  % 1000) * 1000;

  int res = lwip_connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (res < 0 && errno != EINPROGRESS) {
    DEBUG_PRINTF("connect on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
    close(sockfd);
    return 0;
  }

  res = select(sockfd + 1, nullptr, &fdset, nullptr, _timeoutMs<0 ? nullptr : &tv);
  if (res < 0) {
    DEBUG_PRINTF("select on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
    close(sockfd);
    return 0;
  } else if (res == 0) {
    DEBUG_PRINTF("select returned due to timeout %d ms for fd %d", _timeoutMs, sockfd);
    close(sockfd);
    return 0;
  } else {
    int sockerr;
    socklen_t len = (socklen_t)sizeof(int);
    res = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &sockerr, &len);

    if (res < 0) {
      DEBUG_PRINTF("getsockopt on fd %d, errno: %d, \"%s\"", sockfd, errno, strerror(errno));
      close(sockfd);
      return 0;
    }

    if (sockerr != 0) {
      DEBUG_PRINTF("socket error on fd %d, errno: %d, \"%s\"", sockfd, sockerr, strerror(sockerr));
      close(sockfd);
      return 0;
    }
  }

#define ROE_WIFICLIENT(x,msg) { if (((x)<0)) { DEBUG_PRINTF("Setsockopt '" msg "'' on fd %d failed. errno: %d, \"%s\"", sockfd, errno, strerror(errno)); return 0; }}
  ROE_WIFICLIENT(setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)),"SO_SNDTIMEO");
  ROE_WIFICLIENT(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)),"SO_RCVTIMEO");

  fcntl( sockfd, F_SETFL, fcntl( sockfd, F_GETFL, 0 ) & (~O_NONBLOCK) );
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
    //use select to make sure the socket is ready for writing
    fd_set set;
    struct timeval tv;
    FD_ZERO(&set);        // empties the set
    FD_SET(socketFD, &set); // adds FD to the set
    tv.tv_sec = 0;
    tv.tv_usec = WIFI_CLIENT_SELECT_TIMEOUT_US;
    retry--;

    if(select(socketFD + 1, NULL, &set, NULL, &tv) < 0) {
      return 0;
    }

    if(FD_ISSET(socketFD, &set)) {
      res = send(socketFD, (void*) buf, bytesRemaining, MSG_DONTWAIT);
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
        // DEBUG_PRINTF("fail on fd %d, errno: %d, \"%s\"", socketFD, errno, strerror(errno));
        if(errno != EAGAIN) {
        //   //if resource was busy, can try again, otherwise give up
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
  int count = 0;
  int res = lwip_ioctl(_sockfd, FIONREAD, &count);
  if (res<0 || count==0) {
    return 0;
  }

  if (nullptr == _rxBuff) {
    _rxBuff = new cbuf(TCP_RX_PACKET_MAX_SIZE);
    if (!_rxBuff) {
      return -1;
    }
  }

  uint8_t *tmpBuff = (uint8_t *)tal_malloc(count);
  if (!tmpBuff) {
    return -1;
  }

  res = recv(_sockfd, tmpBuff, count, MSG_DONTWAIT);
  if(res < 0) {
    return 0;
  }
  _rxBuff->write((const char*)tmpBuff, (size_t)count);
  tal_free(tmpBuff);

  if (!buf && size > 0) {
    _rxBuff->read((char*)buf, size);
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
  close(_sockfd);
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
