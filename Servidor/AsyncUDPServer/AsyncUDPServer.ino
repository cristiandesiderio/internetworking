/**
 * @file AsyncUDPServer.ino
 * @brief This file contains the code for an asynchronous UDP server that handles various commands and communicates with multiple devices.
 */

#include "WiFi.h"
#include "AsyncUDP.h"
#include "WiFiUdp.h"

// Constants
#define PORT 9999
#define MAX_NAME_LENGTH 50

// Data structure to store device information
struct Device
{
  String name;  ///< Device name
  IPAddress ip; ///< Device IP address
};

Device devices[16];     ///< Array to store devices
int numDevices = 0;     ///< Number of devices connected to the server
String serverName = ""; ///< Name of the server

uint8_t LED2pin = 21;  ///< Pin number for LED2
bool LED2status = LOW; ///< Status of LED2

const char *ssid = "Hipermegared 2.4GHz"; ///< WiFi SSID
const char *password = "0043130185";      ///< WiFi password

AsyncUDP udp; ///< AsyncUDP object for handling UDP communication

/**
 * @brief Sends an asynchronous UDP request.
 * @param message The message to be sent.
 * @param targetIP The IP address of the target device.
 * @param targetPort The port number of the target device.
 * @return The response received from the target device.
 */
String sendUDPRequest(const char *message, IPAddress targetIP, int targetPort)
{
  WiFiUDP udpInstance; // Use the synchronous WiFiUDP class
  udpInstance.begin(10001);
  // Send the request using the provided WiFiUDP instance
  udpInstance.beginPacket(targetIP, targetPort);
  udpInstance.print(message);
  udpInstance.endPacket();
  Serial.println("Synchronous UDP request sent");
  // Wait for the actual response
  int timeout = 5000; // Adjust as needed
  unsigned long startTime = millis();
  while (millis() - startTime < timeout)
  {
    Serial.println("Waiting for UDP response...");
    if (udpInstance.parsePacket())
    {
      // Response received, process it
      Serial.println("UDP response received");
      char response[1024];
      udpInstance.read(response, sizeof(response));
      response[udpInstance.available()] = '\0';
      return response;
    }
  }

  // Timeout
  return "501 timeout";
}

/**
 * @brief Validates if a device exists in the list of devices.
 * @param nameToActivate The name of the device to validate.
 * @return The index of the device if found, -1 otherwise.
 */
int validateDevice(String nameToActivate)
{
  for (int i = 0; i < numDevices; ++i)
  {
    if (devices[i].name == nameToActivate)
    {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Internal handler for the SET method.
 * @param key The key to set.
 * @param value The value to set.
 * @return True if the SET method was handled successfully, false otherwise.
 */
bool onSetHandler(String key, String value)
{
  if (key == "LED")
  {
    if (value == "ON")
    {
      digitalWrite(LED2pin, HIGH);
    }
    else if (value == "OFF")
    {
      digitalWrite(LED2pin, LOW);
    }
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Internal handler for the GET method.
 * @param key The key to get.
 * @return The value corresponding to the key.
 */
String onGETHandler(String key)
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
 * @brief Extracts the device name from the packet data.
 * @param packetData The packet data.
 * @param start The starting index to search for the device name.
 * @return The extracted device name.
 */
String deviceName(String packetData, int start)
{
  if (packetData.indexOf(' ', 1) != -1)
  {
    return packetData.substring(packetData.indexOf(' ', start) + 1);
  }
  else
  {
    return packetData;
  }
}

/**
 * @brief Handles the SET method.
 * @param nameToActivate The name of the device to activate.
 * @param key The key to set.
 * @param value The value to set.
 * @param packet The AsyncUDPPacket object.
 */
void handleSETMethod(String nameToActivate, String key, String value, AsyncUDPPacket packet)
{
  if (nameToActivate == serverName || nameToActivate.isEmpty())
  {
    Serial.println("Activating local device");
    if (onSetHandler(key, value))
    {
      packet.printf("200 OK");
    }
    else
    {
      packet.printf("400 Bad Request");
    }
  }
  else if (validateDevice(nameToActivate) != -1)
  {
    IPAddress ip = devices[validateDevice(nameToActivate)].ip;
    String requestStr = "SET " + key + " " + value;
    String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
    Serial.printf("Response from device '%s': %s\n", nameToActivate, response.c_str());
    packet.printf(response.c_str());
  }
  else
  {
    String requestStr = "SET " + key + " " + value;
    bool found = false;
    String response;
    for (int i = 0; i < numDevices; ++i)
    {
      IPAddress ip = devices[i].ip;
      String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
      Serial.printf("Response from device '%s': %s\n", devices[i].name.c_str(), response.c_str());
      packet.printf(response.c_str());
      if (response.startsWith("200"))
      {
        found = true;
        break;
      }
    }

    if (found)
    {
      packet.printf(response.c_str());
    }
    else
    {
      packet.printf("404 Not Found");
    }
  }
}

/**
 * @brief Handles the GET method.
 * @param nameToActivate The name of the device to activate.
 * @param key The key to get.
 * @param packet The AsyncUDPPacket object.
 */
void handleGETMethod(String nameToActivate, String key, AsyncUDPPacket packet)
{
  if (nameToActivate == serverName || nameToActivate.isEmpty())
  {
    Serial.println("Getting from local device");
    String response = onGETHandler(key);
    packet.printf("200 OK %s", response.c_str());
  }
  else if (validateDevice(nameToActivate) != -1)
  {
    IPAddress ip = devices[validateDevice(nameToActivate)].ip;
    String requestStr = "GET " + key;
    String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
    Serial.printf("Response from device '%s': %s\n", nameToActivate, response.c_str());
    packet.printf(response.c_str());
  }
  else
  {
    String requestStr = "GET " + nameToActivate + " " + key;
    bool found = false;
    String response;
    for (int i = 0; i < numDevices; ++i)
    {
      IPAddress ip = devices[i].ip;
      response = sendUDPRequest(requestStr.c_str(), ip, 9999);
      Serial.printf("Response from device '%s': %s\n", devices[i].name.c_str(), response.c_str());
      if (response.startsWith("200"))
      {
        found = true;
        break;
      }
    }

    if (found)
    {
      packet.printf(response.c_str());
    }
    else
    {
      packet.printf("404 Not Found");
    }
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
  pinMode(LED2pin, OUTPUT);
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

  // Start UDP server
  if (udp.listen(9999))
  {
    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

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

      if (packetData.startsWith("NAME"))
      {
        // NAME message: Store the device name
        int spaceIndex = packetData.indexOf(' ');
        if (spaceIndex != -1)
        {
          String name = packetData.substring(spaceIndex + 1);
          Serial.printf("Device name '%s' received.\n", name.c_str());
          serverName = name;
          Serial.printf("Device name '%s' stored.\n", serverName.c_str());
          packet.printf("200 OK %s", serverName.c_str());
        }
        else
        {
          // No space, no name provided
          Serial.println("No name provided in the SET command");
          packet.printf("400 Bad Request");
        }
      }
      else if (packetData.startsWith("ADD"))
      {
        // ADD message: Add a new device
        if (numDevices == 16)
        {
          packet.printf("400 Max Devices Reached");
        }

        String name = packetData.substring(4, packetData.indexOf(' ', 4));
        String ipString = packetData.substring(packetData.indexOf(' ', 4) + 1);

        // Convert IP address from string to IPAddress
        IPAddress ip;
        if (ip.fromString(ipString))
        {
          devices[numDevices].name = name;
          devices[numDevices].ip = ip;
          numDevices++;

          Serial.printf("New device added - Name: '%s', IP: '%s'\n", name.c_str(), ipString.c_str());
          packet.printf("200 OK %s", serverName);
        }
        else
        {
          Serial.println("Error converting IP address.");
          packet.printf("400 Bad Request");
        }
      }
      else if (packetData.startsWith("RM"))
      {
        // RM message: Remove a device
        String nameToRemove = packetData.substring(packetData.indexOf(' ', 1) + 1);

        for (int i = 0; i < numDevices; ++i)
        {
          if (devices[i].name == nameToRemove)
          {
            // Remove the found device by shifting the remaining elements
            for (int j = i; j < numDevices - 1; ++j)
            {
              devices[j] = devices[j + 1];
            }
            numDevices--;

            Serial.printf("Device '%s' removed.\n", nameToRemove);
            packet.printf("200 OK %s", serverName);
            return;
          }
        }

        // Device not found
        Serial.printf("Device '%s' not found.\n", nameToRemove);
        packet.printf("404 Not Found");
      }
      else if (packetData.startsWith("LIST"))
      {
        // LIST message: Return the list of devices
        String response = "200 OK\n";

        for (int i = 0; i < numDevices; ++i)
        {
          response += devices[i].name + " " + devices[i].ip.toString() + "\n";
        }

        Serial.printf("Sending list of devices:\n%s", response.c_str());
        packet.printf(response.c_str());
      }
      else if (packetData.startsWith("SET"))
      {
        const char *splitted = strtok((char *)packetData.c_str(), " ");
        // Save all tokens in an array
        char *tokens[4];
        int index = 0;
        while (splitted != NULL)
        {
          tokens[index] = const_cast<char *>(splitted);
          splitted = strtok(NULL, " ");
          index++;
        }

        if (index != 4)
        {
          Serial.println("400 Bad Request");
          packet.printf("400 Bad Request");
          return;
        }

        handleSETMethod(tokens[1], tokens[2], tokens[3], packet);
      }
      else if (packetData.startsWith("GET"))
      {
        const char *splitted = strtok((char *)packetData.c_str(), " ");
        // Save all tokens in an array
        char *tokens[3];
        int index = 0;
        while (splitted != NULL)
        {
          tokens[index] = const_cast<char *>(splitted);
          splitted = strtok(NULL, " ");
          index++;
        }

        if (index != 4)
        {
          Serial.println("400 Bad Request");
          packet.printf("400 Bad Request");
          return;
        }

        handleGETMethod(tokens[1], tokens[2], packet);
      }
      else if (packetData.startsWith("WHO"))
      {
        char ipString[20];
        IPAddress ip = WiFi.localIP();
        snprintf(ipString, sizeof(ipString), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

        if (!serverName.isEmpty())
        {
          // WHO message: Respond with IP and device name
          Serial.printf("Device name '%s'\n", serverName);
          packet.printf("200 OK %s %s", ipString, serverName);
        }
        else
        {
          packet.printf("200 OK %s %s", ipString, "Name not set");
        }
      }
      else
      {
        Serial.printf("Method Not Allowed");
        packet.printf("405 Method Not Allowed");
      } });
  }
}

/**
 * @brief The loop function.
 */
void loop()
{
  delay(300);
}
