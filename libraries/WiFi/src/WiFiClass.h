#ifndef __WIFI_CLASS_H__
#define __WIFI_CLASS_H__

#include <Arduino.h>
#include <stdint.h>
#include "WiFiType.h"

class WiFiClass{
private:
    /* data */
public:
    WiFiClass();
    ~WiFiClass();

    uint8_t channel(void);

    bool mode(WiFiMode_t mode);
    WiFiMode_t getMode();

    bool enableSTA(bool enable);
    bool enableAP(bool enable);

    bool setSleep(bool enabled);

    IPAddress localIP();

    /* wifi station */
    int begin(const char* ssid); // open networks
    int begin(const char* ssid, const char *password);

    bool connected();
    bool isConnected();

    bool disconnect();

    /* wifi softAP */
    bool beginAP(const char *ssid);
    bool beginAP(const char *ssid, uint8_t channel);
    bool beginAP(const char *ssid, const char* password);
    bool beginAP(const char *ssid, const char* password, uint8_t channel);

    bool softAPdisconnect();
};

extern WiFiClass WiFi;

#endif // __WIFI_CLASS_H__
