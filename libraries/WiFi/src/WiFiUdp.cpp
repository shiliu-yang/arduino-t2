#include "WiFiUdp.h"

#include "tal_memory.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"

extern "C" void bk_printf(const char *fmt, ...);

#define DEBUG_PRINTF bk_printf

#undef write
#undef read

WiFiUdp::WiFiUdp()
  : _udpSocket(-1)
  , _port(0)
  , _remotePort(0)
  , _rxBuff(NULL)
  , _txBuff(NULL)
  , _txBuffLen(0)
{
}

WiFiUdp::~WiFiUdp()
{
  stop();
}

uint8_t WiFiUdp::begin(uint16_t port)
{
  begin(IPAddress((uint32_t)INADDR_ANY), port);
  return 1;
}

uint8_t WiFiUdp::begin(IPAddress a, uint16_t p)
{
  stop();

  _port = p;

  _txBuff = (char *)malloc(UDP_TX_PACKET_MAX_SIZE);
  if(!_txBuff){
    DEBUG_PRINTF("could not create tx buffer: %d", errno);
    return 0;
  }

  if ((_udpSocket=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    DEBUG_PRINTF("could not create socket: %d", errno);
    return 0;
  }

  int yes = 1;
  if (setsockopt(_udpSocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
      DEBUG_PRINTF("could not set socket option: %d", errno);
      stop();
      return 0;
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_port);
  addr.sin_addr.s_addr = (in_addr_t)a;
  if(bind(_udpSocket , (struct sockaddr*)&addr, sizeof(addr)) == -1){
    DEBUG_PRINTF("could not bind socket: %d", errno);
    stop();
    return 0;
  }
  fcntl(_udpSocket, F_SETFL, O_NONBLOCK);
  return 1;
}

uint8_t WiFiUdp::beginMulticast(IPAddress a, uint16_t p)
{
  if(begin(IPAddress((uint32_t)INADDR_ANY), p)){
    if((uint32_t)a != 0){
      struct ip_mreq mreq;
      mreq.imr_multiaddr.s_addr = (uint32_t)a;
      mreq.imr_interface.s_addr = (in_addr_t)INADDR_ANY;
      if (setsockopt(_udpSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
          DEBUG_PRINTF("could not join igmp: %d", errno);
          stop();
          return 0;
      }
      _multicastIP = a;
    }
    return 1;
  }
  return 0;
}

void WiFiUdp::stop()
{
  if (_txBuff) {
    tal_free(_txBuff);
    _txBuff = NULL;
  }
  _txBuffLen = 0;

  if (_rxBuff) {
    cbuf *b = _rxBuff;
    _rxBuff = NULL;
    delete b;
  }

  if (_udpSocket == -1) {
    return;
  }

  if((uint32_t)_multicastIP != 0){
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = (uint32_t)_multicastIP;
    mreq.imr_interface.s_addr = (in_addr_t)0;
    setsockopt(_udpSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    _multicastIP = IPAddress((uint32_t)INADDR_ANY);
  }
  close(_udpSocket);
  _udpSocket = -1;
}

int WiFiUdp::beginPacket(IPAddress ip, uint16_t port)
{
  _remoteIP = ip;
  _remotePort = port;

  if(!_remotePort){
    return 0;
  }

  // allocate tx_buffer if is necessary
  if(!_txBuff){
    _txBuff = (char *)malloc(1460);
    if(!_txBuff){
      DEBUG_PRINTF("could not create tx buffer: %d", errno);
      return 0;
    }
  }

  _txBuffLen = 0;

  // check whereas socket is already open
  if (_udpSocket != -1)
    return 1;

  if ((_udpSocket=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    DEBUG_PRINTF("could not create socket: %d", errno);
    return 0;
  }

  fcntl(_udpSocket, F_SETFL, O_NONBLOCK);

  return 1;
}

int WiFiUdp::beginPacket(const char *host, uint16_t port)
{
  struct hostent *server;
  server = gethostbyname(host);
  if (server == NULL){
    DEBUG_PRINTF("could not get host from dns: %d", errno);
    return 0;
  }
  return beginPacket(IPAddress((const char *)(server->h_addr_list[0])), port);
}

int WiFiUdp::endPacket()
{
  struct sockaddr_in recipient;
  recipient.sin_addr.s_addr = (uint32_t)_remoteIP;
  recipient.sin_family = AF_INET;
  recipient.sin_port = htons(_remotePort);
  int sent = sendto(_udpSocket, _txBuff, _txBuffLen, 0, (struct sockaddr*) &recipient, sizeof(recipient));
  if(sent < 0){
    DEBUG_PRINTF("could not send data: %d", errno);
    return 0;
  }
  return 1;
}

size_t WiFiUdp::write(uint8_t byte)
{
  if(_txBuffLen == UDP_TX_PACKET_MAX_SIZE){
    endPacket();
    _txBuffLen = 0;
  }
  _txBuff[_txBuffLen++] = (char)byte;
  return 1;
}

size_t WiFiUdp::write(const uint8_t *buffer, size_t size)
{
  size_t i;
  for(i=0; i<size; i++) {
    write(buffer[i]);
  }
  return i;
}

int WiFiUdp::parsePacket()
{
  if(_rxBuff)
    return 0;
  struct sockaddr_in si_other;
  int slen = sizeof(si_other) , len;
  char *buf = (char *)malloc(UDP_RX_PACKET_MAX_SIZE);
  if(!buf) {
    return 0;
  }
  if ((len = recvfrom(_udpSocket, buf, UDP_RX_PACKET_MAX_SIZE, MSG_DONTWAIT, (struct sockaddr *) &si_other, (socklen_t *)&slen)) == -1){
    free(buf);
    if(errno == EWOULDBLOCK){
      return 0;
    }
    DEBUG_PRINTF("could not receive data: %d", errno);
    return 0;
  }
  _remoteIP = IPAddress(si_other.sin_addr.s_addr);
  _remotePort = ntohs(si_other.sin_port);
  if (len > 0) {
    _rxBuff = new cbuf(len);
    _rxBuff->write(buf, len);
  }
  free(buf);
  return len;
}
int WiFiUdp::available()
{
  if(!_rxBuff) return 0;
  return _rxBuff->available();
}
int WiFiUdp::read()
{
  if(!_rxBuff) return -1;
  int out = _rxBuff->read();
  if(!_rxBuff->available()){
    cbuf *b = _rxBuff;
    _rxBuff = 0;
    delete b;
  }
  return out;
}
int WiFiUdp::read(unsigned char* buffer, size_t len)
{
  return read((char *)buffer, len);
}
int WiFiUdp::read(char* buffer, size_t len)
{
  if(!_rxBuff) return 0;
  int out = _rxBuff->read(buffer, len);
  if(!_rxBuff->available()){
    cbuf *b = _rxBuff;
    _rxBuff = 0;
    delete b;
  }
  return out;
}
int WiFiUdp::peek()
{
  if(!_rxBuff) return -1;
  return _rxBuff->peek();
}
void WiFiUdp::flush()
{
  if(!_rxBuff) return;
  cbuf *b = _rxBuff;
  _rxBuff = 0;
  delete b;
}

IPAddress WiFiUdp::remoteIP()
{
  return _remoteIP;
}
uint16_t WiFiUdp::remotePort()
{
  return _remotePort;
}
