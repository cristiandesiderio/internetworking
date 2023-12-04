#include "Domotic.h"
#include "WiFiUdp.h"

Device devices[16];     ///< Array to store devices
int numDevices = 0;     ///< Number of devices connected to the server
String serverName = ""; ///< Name of the server
IPAddress serverIp;     ///< IP address of the server
IPAddress broadcastIP;  ///< Broadcast IP address

bool notImplementedSetHandler(String key, String value)
{
    return false;
}

String notImplementedGetHandler(String key, String value)
{
    return "Set Not Implemented";
}

// Callback function for pin management
SetCallBack setCallback = SetCallBack(notImplementedSetHandler);

void setSetCallback(SetCallBack callback)
{
    setCallback = callback;
}

GetCallBack getCallback = GetCallBack(notImplementedGetHandler);

void setGetCallback(GetCallBack callback)
{
    getCallback = callback;
}

void trim(char *str)
{
    while (isspace(*str) || *str == '\n' || *str == '\r')
    {
        str++;
    }
    size_t len = strlen(str);
    while (len > 0 && (isspace(str[len - 1]) || str[len - 1] == '\n' || str[len - 1] == '\r' || str[len - 1] == '\0'))
    {
        len--;
    }
}

String sendUDPRequest(const char *message, IPAddress targetIP, int targetPort)
{
    WiFiUDP udpInstance;
    if (udpInstance.begin(1001))
    {
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

                // Dynamically allocate memory for response
                uint16_t packetSize = udpInstance.available();
                char *response = new char[packetSize + 1];

                if (response)
                {
                    udpInstance.read(response, packetSize);
                    response[packetSize] = '\0'; // Null-terminate the response
                    Serial.printf("UDP response: %s\n", response);
                    return response;
                }
                else
                {
                    Serial.println("Memory allocation for response failed");
                    return "500 Internal Server Error"; // You may want to handle this case more gracefully
                }
            }
        }

        // Timeout
        Serial.println("UDP request timeout");
        return "501 timeout";
    }
    else
    {
        Serial.println("UDP instance failed");
        return "500 Internal Server Error";
    }
}

// Validate if a device is connected to the server
int validateDevice(String nameToActivate)
{
    for (int i = 0; i < numDevices; i++)
    {
        if (devices[i].name == nameToActivate)
        {
            return i;
        }
    }
    return -1;
}

// deviceName
String deviceName(String packetData, int start)
{
    String name = "";
    for (int i = start; i < packetData.length(); i++)
    {
        if (packetData[i] == ' ')
        {
            break;
        }
        name += packetData[i];
    }
    return name;
}

// Handles the SET method
String handleSETMethod(String nameToActivate, String key, String value)
{
    if (nameToActivate.equals(serverName))
    {
        Serial.println("Activating local device");
        if (setCallback == NULL)
        {
            return "500 No callback function for SET method";
        }
        else
        {
            if (setCallback(key, value))
            {
                return "200 OK";
            }
            else
            {
                return "400 Bad Request";
            }
        }
    }
    else if (validateDevice(nameToActivate) != -1)
    {
        Serial.println("Activating remote device");
        IPAddress ip = devices[validateDevice(nameToActivate)].ip;
        String requestStr = "SET " + nameToActivate + " " + key + " " + value;
        String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
        Serial.printf("Response from device '%s': %s\n", nameToActivate, response.c_str());
        return response.c_str();
    }
    else
    {
        String requestStr = "SET " + nameToActivate + " " + key + " " + value;
        String response;
        for (int i = 0; i < numDevices; ++i)
        {
            Serial.printf("Sending request to device '%s': %s\n", devices[i].name.c_str(), requestStr.c_str());
            IPAddress ip = devices[i].ip;
            String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
            Serial.printf("Response from device '%s': %s\n", devices[i].name.c_str(), response.c_str());

            if (response.startsWith("200"))
            {
                return response.c_str();
            }
        }

        return "404 Not Found";
    }
}

// Handles the GET method
String handleGETMethod(String nameToActivate, String key)
{
    if (nameToActivate.equals(serverName))
    {
        Serial.println("Activating local device");
        if (setCallback == NULL)
        {
            return "500 No callback function for GET method";
        }
        else
        {
            String value = getCallback(key);
            return "200 OK " + value;
        }
    }
    else if (validateDevice(nameToActivate) != -1)
    {
        Serial.println("Activating remote device");
        IPAddress ip = devices[validateDevice(nameToActivate)].ip;
        String requestStr = "GET " + nameToActivate + " " + key;
        String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
        Serial.printf("Response from device '%s': %s\n", nameToActivate, response.c_str());
        return response.c_str();
    }
    else
    {
        String requestStr = "GET " + nameToActivate + " " + key;
        String response;
        for (int i = 0; i < numDevices; ++i)
        {
            Serial.printf("Sending request to device '%s': %s\n", devices[i].name.c_str(), requestStr.c_str());
            IPAddress ip = devices[i].ip;
            String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
            Serial.printf("Response from device '%s': %s\n", devices[i].name.c_str(), response.c_str());

            if (response.startsWith("200"))
            {
                return response.c_str();
            }
        }

        return "404 Not Found";
    }
}

// Handles the NAME method
String handleNameMethod(String packetData)
{
    int spaceIndex = packetData.indexOf(' ');
    if (spaceIndex != -1)
    {
        String name = packetData.substring(spaceIndex + 1);
        Serial.printf("Device name '%s' received.\n", name.c_str());
        serverName = name;
        Serial.printf("Device name '%s' stored.\n", serverName.c_str());
        return "200 OK %s", serverName.c_str();
    }
    else
    {
        Serial.println("No name provided in the NAME command");
        return "400 Bad Request";
    }
}

// Handles the WHO method
String handleWhoMethod()
{
    if (!serverName.isEmpty())
    {
        String conf = "Server name: " + serverName + "\n";
        conf += "Server IP: " + serverIp.toString() + "\n";
        conf += "Broadcast IP: " + broadcastIP.toString() + "\n";
        conf += "Number of dispositivos: " + String(numDevices) + "\n";
        conf += "Dispositivos:\n";
        for (int i = 0; i < numDevices; ++i)
        {
            conf += devices[i].name + " " + devices[i].ip.toString() + "\n";
        }

        return "200 OK %s", conf;
    }
    else
    {
        return "200 OK Name not set";
    }
}

String handleADDMethod(String packetData)
{
    String name = packetData.substring(4, packetData.indexOf(' ', 4));
    String ipString = packetData.substring(packetData.indexOf(' ', 4) + 1);
    trim((char *)(name).c_str());
    if (name.length() > 0 && ipString.length() > 0)
    {
        if (name.equals(serverName))
        {
            return "203 Name already in use";
        }

        IPAddress ip;
        if (ip.fromString(ipString))
        {

            String requestStr = "ADD " + serverName + " " + serverIp;
            String response = sendUDPRequest(requestStr.c_str(), ip, 9999);

            if (response.startsWith("200"))
            {
                devices[numDevices].name = name;
                devices[numDevices].ip = ip;
                numDevices++;
                Serial.printf("New device added - Name: '%s', IP: '%s'\n", name.c_str(), ipString.c_str());
                return "200 OK " + serverName;
            }
            else
            {
                Serial.printf("500 Error adding device '%s': %s\n", name.c_str(), response.c_str());
                return "500 Error adding device";
            }
        }
        else
        {
            Serial.println("Error converting IP address.");
            return "400 Bad Request";
        }
    }
    else
    {
        return "400 Bad Request";
    }
}

String handleDELMethod(String packetData)
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
            return "200 OK %s", serverName;
        }
    }

    // Device not found
    Serial.printf("Device '%s' not found.\n", nameToRemove);
    return "404 Not Found";
}

String handleListMethod()
{
    String list = "Dispositivos:\n";
    for (int i = 0; i < numDevices; ++i)
    {
        list += devices[i].name + " " + devices[i].ip.toString() + "\n";
    }

    return list;
}

// handles processUDPPacket receives the whole packet and processes it
String processUDPPacket(String packetData, size_t length)
{
    // remove breakline and trim packetdata
    packetData.replace("\n", "");
    packetData.replace("\r", "");
    if (packetData.charAt(packetData.length() - 1) == '\0')
    {
        packetData.remove(packetData.length() - 1);
    }

    trim((char *)packetData.c_str());

    String returnMessage = "";
    if (packetData.startsWith("NAME"))
    {
        returnMessage = handleNameMethod(packetData);
    }
    else if (packetData.startsWith("WHO"))
    {
        // WHO message: Respond with IP and device name
        returnMessage = handleWhoMethod();
    }
    else if (packetData.startsWith("SET"))
    {
        // SET message: Set a value
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
            return "400 Bad Request";
        }

        returnMessage = handleSETMethod(tokens[1], tokens[2], tokens[3]);
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

        if (index != 3)
        {
            Serial.println("400 Bad Request");
            return "400 Bad Request";
        }

        returnMessage = handleGETMethod(tokens[1], tokens[2]);
    }
    else if (packetData.startsWith("PING"))
    {
        returnMessage = "200 OK";
    }
    else if (packetData.startsWith("ADD"))
    {
        // ADD message: Add a new device
        if (numDevices == 16)
        {
            return "400 Max Devices Reached";
        }

        returnMessage = handleADDMethod(packetData);
    }
    else if (packetData.startsWith("DEL"))
    {
        if (numDevices == 0)
        {
            return "400 No devices to remove";
        }

        returnMessage = handleDELMethod(packetData);
    }
    else if (packetData.startsWith("LIST"))
    {
        returnMessage = handleListMethod();
    }
    else
    {
        Serial.println("400 Bad Request");
        return "400 Bad Request";
    }

    Serial.println(returnMessage);
    return returnMessage;
}
