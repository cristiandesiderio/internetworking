#include "Domotic.h"
#include "WiFiUdp.h"

Device devices[16];     ///< Array to store devices
int numDevices = 0;     ///< Number of devices connected to the server
String serverName = ""; ///< Name of the server

// Callback function for pin management
SetCallBack setCallback = nullptr;

void setSetCallback(SetCallBack callback)
{
    setCallback = callback;
}

GetCallBack getCallback = nullptr;

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
    if (nameToActivate == serverName)
    {
        if (getCallback != NULL)
        {
            String value = getCallback(key);
            return "200 OK " + value;
        }
        else
        {
            return "500 No callback function for GET method";
        }
    }
    else
    {
        int deviceIndex = validateDevice(nameToActivate);
        if (deviceIndex != -1)
        {
            sendUDPRequest(("GET " + nameToActivate + " " + key).c_str(), devices[deviceIndex].ip, 5000);
        }
        else
        {
            Serial.println("Device not found");
        }
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
        Serial.println("No name provided in the SET command");
        return "400 Bad Request";
    }
}

// Handles the WHO method
String handleWhoMethod(String packetData)
{

    if (!serverName.isEmpty())
    {
        // WHO message: Respond with IP and device name
        Serial.printf("Device name '%s'\n", serverName);
        return "200 OK %s", serverName.c_str();
    }
    else
    {
        return "200 OK Name not set";
    }
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
        returnMessage = handleWhoMethod(packetData);
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
        // GET message: Get a value
        int spaceIndex = packetData.indexOf(' ');
        if (spaceIndex != -1)
        {
            String nameToActivate = deviceName(packetData, spaceIndex + 1);
            int spaceIndex2 = packetData.indexOf(' ', spaceIndex + 1);
            if (spaceIndex2 != -1)
            {
                String key = packetData.substring(spaceIndex2 + 1);
                returnMessage = handleGETMethod(nameToActivate, key);
            }
            else
            {
                Serial.println("No key provided in the GET command");
            }
        }
        else
        {
            Serial.println("No name provided in the GET command");
        }
    }

    return returnMessage;

    //   if (method == "SET")
    //   {

    //     return handleSETMethod(nameToActivate, key, value);
    //   }
    //   else if (method == "GET")
    //   {
    //     return handleGETMethod(nameToActivate, key);
    //   }
    //   else if (method == "NAME")
    //   {
    //     return handleName(packetData);
    //   }
    //   else
    //   {
    //     Serial.println("Invalid method");
    //   }
}
