/**
 * @file Servidor.ino
 * @brief This file contains the code for an asynchronous UDP server that handles various commands and communicates with multiple devices.
 */
#include "WiFi.h"
#include "WiFiUdp.h"
#include "Domotic.h"

const int LED21pin = 21;

const char *ssid = "Cant-touch-this";
const char *password = "necochea1629";
AsyncUDP udp;

/**
 * @brief Internal handler for the SET method.
 * @param key The key to set.
 * @param value The value to set.
 * @return True if the SET method was handled successfully, false otherwise.
 */
bool localSetHandler(String key, String value)
{
  Serial.printf("Local SET '%s' %s\n", key.c_str(), value.c_str());
  int selectedLED = 21;
  if(key == "LED21") { selectedLED = LED21pin; }
  else { return false; }

  if (value == "ON")
  {
    digitalWrite(selectedLED, HIGH);
    return true;
  }
  else if (value == "OFF")
  {
    digitalWrite(selectedLED, LOW);
    return true;
  }
  else
  {
    return false;
  }


}


String localOptionsHandler()
{
  return "SET LED21 (ON/OFF) \n GET TEMP \n";
}

/**
 * @brief Internal handler for the GET method.
 * @param key The key to get.
 * @return The value corresponding to the key.
 */
String localGETHandler(String key)
{
  if (key == "TEMP")
  {
    // Return a random simulated value between 20 and 35
    int value = random(20, 35);
    Serial.printf("Temperature value: %d\n", value);
    return String(value);
  }
  else
  {
    return "Undefined value";
  }
}

/**
 * @brief The setup function.
 */
void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Set pin modes
  pinMode(21, OUTPUT);
  pinMode(2, OUTPUT);

  // Connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi network...");
  }
  Serial.println("WiFi connection established");
  Serial.print("STA IP address: ");
  Serial.println(WiFi.localIP());
  serverName = WiFi.macAddress();
  serverIp = WiFi.localIP();
  broadcastIP = WiFi.broadcastIP();
  // Start UDP server
  if (udp.listen(9999))
  {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    setSetCallback(SetCallBack(localSetHandler));
    setGetCallback(GetCallBack(localGETHandler));
    setOptionsCallback(OptionsCallBack(localOptionsHandler));
    
    // Handle incoming packets
    udp.onPacket([](AsyncUDPPacket packet)
                 {
      digitalWrite(2, HIGH);
      delay(300);
      digitalWrite(2, LOW);
      Serial.print("UDP Packet Type: ");
      Serial.print(", From: ");
      Serial.print(packet.remoteIP());
      Serial.print(":");
      Serial.print(packet.remotePort());
      Serial.print(", To: ");
      Serial.print(packet.localIP());
      Serial.print(":");
      Serial.print(packet.localPort());
      Serial.print(", Length: ");
      Serial.print(packet.length());
      Serial.print(", Data: ");
      Serial.write(packet.data(), packet.length());
      Serial.println();

      String packetData = packet.readString();

      String returnMessage = processUDPPacket(packetData);
      packet.printf(returnMessage.c_str());
   });
  }
  else
  {
    Serial.println("Failed to start UDP server");
  }
}

/**
 * @brief The loop function.
 */ 
void loop()
{
  delay(200);
}
