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

extern Device devices[16];     ///< Array to store devices
extern int numDevices;     ///< Number of devices connected to the server
extern String serverName; ///< Name of the server
extern IPAddress serverIp; ///< IP address of the server
extern IPAddress broadcastIP; ///< Broadcast IP address

typedef bool (*SetCallBack)(const String& key, const String& value);
typedef String (*GetCallBack)(const String& key);


/**
 * @brief Envía una solicitud UDP asíncrona.
 * @param udpInstance Puntero a la instancia AsyncUDP.
 * @param message El mensaje a enviar.
 * @param targetIP La dirección IP del dispositivo objetivo.
 * @param targetPort El número de puerto del dispositivo objetivo.
 * @return La respuesta recibida del dispositivo objetivo.
 */
void setSetCallback(SetCallBack callback);
void setGetCallback(GetCallBack callback);
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
