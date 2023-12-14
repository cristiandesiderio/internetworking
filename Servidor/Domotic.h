// Domotic.h

#ifndef DOMOTIC_H
#define DOMOTIC_H

#include "Arduino.h"
#include "AsyncUDP.h"


#define MAX_NAME_LENGTH 50

struct Device
{
  String name;
  IPAddress ip;
};

extern Device devices[16];     /// Array de dispositivos conectados al servidor
extern int numDevices;         /// Numero de dispositivos conectados al servidor
extern String serverName; /// Nombre del servidor
extern IPAddress serverIp; /// IP del servidor
extern IPAddress broadcastIP; /// IP de broadcast

typedef bool (*SetCallBack)(const String& key, const String& value);
typedef String (*GetCallBack)(const String& key);
typedef String (*OptionsCallBack)();



void setSetCallback(SetCallBack callback);
void setGetCallback(GetCallBack callback);
void setOptionsCallback(OptionsCallBack callback);
String sendUDPRequest(const char *message, IPAddress targetIP, int targetPort);
int validateDevice(String nameToActivate);
String handleSETMethod(String nameToActivate, String key, String value);
String handleGETMethod(String nameToActivate, String key);
String handleNameMethod(String packetData);
String handleWhoMethod();
String handleADDMethod(String packetData);
String handleDELMethod(String packetData);
String handleListMethod();
String processUDPPacket(String data);

#endif // DOMOTIC_H
