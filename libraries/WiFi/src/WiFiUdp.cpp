#include "WiFiUdp.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "errno.h"

extern "C" void bk_printf(const char *fmt, ...);

#undef write
#undef read

WiFiUdp::WiFiUdp()
{
}

WiFiUdp::~WiFiUdp()
{
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

  // tx_buffer = (char *)malloc(UDP_TX_PACKET_MAX_SIZE);
  // if(!tx_buffer){
  //   bk_printf("could not create tx buffer: %d", errno);
  //   return 0;
  // }

  if ((_udpSocket=socket(AF_INET, SOCK_DGRAM, 0)) == -1){
    bk_printf("could not create socket: %d", errno);
    return 0;
  }

  int yes = 1;
  if (setsockopt(_udpSocket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
      bk_printf("could not set socket option: %d", errno);
      stop();
      return 0;
  }

  struct sockaddr_in addr;
  memset((char *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(_port);
  addr.sin_addr.s_addr = (in_addr_t)a;
  if(bind(_udpSocket , (struct sockaddr*)&addr, sizeof(addr)) == -1){
    bk_printf("could not bind socket: %d", errno);
    stop();
    return 0;
  }
  fcntl(_udpSocket, F_SETFL, O_NONBLOCK);
  return 1;
}

uint8_t WiFiUdp::beginMulticast(IPAddress a, uint16_t p)
{
  stop();

  return 1;
}

void WiFiUdp::stop()
{
  
}

int WiFiUdp::beginPacket(IPAddress ip, uint16_t port)
{
  return 0;
}

int WiFiUdp::beginPacket(const char *host, uint16_t port)
{
  return 0;
}

int WiFiUdp::endPacket()
{
  return 0;
}

size_t WiFiUdp::write(uint8_t byte)
{
  return 0;
}

size_t WiFiUdp::write(const uint8_t *buffer, size_t size)
{
  return 0;
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
    bk_printf("could not receive data: %d", errno);
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
