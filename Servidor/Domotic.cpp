#include "Domotic.h"
#include "WiFiUdp.h"

Device devices[16];
int numDevices = 0;
String serverName = "";
IPAddress serverIp;
IPAddress broadcastIP;

/**
 * @brief Comportamiento por defecto metodo SET.
*/
bool notImplementedSetHandler(String key, String value)
{
    return false;
}

/**
 * @brief Comportamiento por defecto metodo GET.
*/
String notImplementedGetHandler(String key, String value)
{
    return "Get Not Implemented";
}

/**
 * @brief Comportamiento por defecto metodo OPTIONS.
*/
String notImplementedOptionsHandler()
{
    return "Options Not Implemented";
}



OptionsCallBack optionsCallback = OptionsCallBack(notImplementedOptionsHandler);

/**
 * @brief Setea el comportamiento del metodo OPTIONS.
 * @param callback Funcion que se ejecutara cuando se reciba un mensaje OPTIONS.
*/
void setOptionsCallback(OptionsCallBack callback)
{
    optionsCallback = callback;
}


SetCallBack setCallback = SetCallBack(notImplementedSetHandler);
/**
 * @brief Setea el comportamiento del metodo SET.
 * @param callback Funcion que se ejecutara cuando se reciba un mensaje SET.
*/
void setSetCallback(SetCallBack callback)
{
    setCallback = callback;
}

GetCallBack getCallback = GetCallBack(notImplementedGetHandler);

/**
 * @brief Setea el comportamiento del metodo GET.
 * @param callback Funcion que se ejecutara cuando se reciba un mensaje GET.
*/
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

/**
 * @brief Envia un mensaje UDP a un dispositivo de forma sincrona
 * @param message Mensaje a enviar
 * @param targetIP IP del dispositivo
 * @param targetPort Puerto del dispositivo
 * @return Respuesta del dispositivo
 */
String sendUDPRequest(const char *message, IPAddress targetIP, int targetPort)
{
    WiFiUDP udpInstance;
    if (udpInstance.begin(1001))
    {

        udpInstance.beginPacket(targetIP, targetPort);
        udpInstance.print(message);
        udpInstance.endPacket();
        Serial.println("Synchronous UDP request sent");

        int timeout = 5000;
        unsigned long startTime = millis();

        while (millis() - startTime < timeout)
        {
            if (udpInstance.parsePacket())
            {
                Serial.println("UDP response received");
                uint16_t packetSize = udpInstance.available();
                char *response = new char[packetSize + 1];

                if (response)
                {
                    udpInstance.read(response, packetSize);
                    response[packetSize] = '\0';
                    return response;
                }
                else
                {
                    Serial.println("Memory allocation for response failed");
                    return "500 Internal Server Error";
                }
            }
        }

        Serial.println("UDP request timeout");
        return "501 timeout";
    }
    else
    {
        Serial.println("UDP instance failed");
        return "500 Internal Server Error";
    }
}

/**
 * @brief Valida si el dispositivo existe en la lista de dispositivos
 * @param deviceName Nombre del dispositivo a validar
*/
int validateDevice(String deviceName)
{
    for (int i = 0; i < numDevices; i++)
    {
        if (devices[i].name == deviceName)
        {
            return i;
        }
    }
    return -1;
}


/**
 * @brief Manejador del metodo SET
 * @param deviceName Nombre del dispositivo a activar
 * @param key Clave a setear
*/
String handleSETMethod(String deviceName, String key, String value)
{
    if (deviceName.equals(serverName))
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
    else if (validateDevice(deviceName) != -1)
    {
        Serial.println("Activating remote device");
        IPAddress ip = devices[validateDevice(deviceName)].ip;
        String requestStr = "SET " + deviceName + " " + key + " " + value;
        String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
        Serial.printf("Response from device '%s': %s\n", deviceName, response.c_str());
        return response.c_str();
    }
    else
    {
        String requestStr = "SET " + deviceName + " " + key + " " + value;
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

String handleGETMethod(String deviceName, String key)
{
    if (deviceName.equals(serverName))
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
    else if (validateDevice(deviceName) != -1)
    {
        Serial.println("Activating remote device");
        IPAddress ip = devices[validateDevice(deviceName)].ip;
        String requestStr = "GET " + deviceName + " " + key;
        String response = sendUDPRequest(requestStr.c_str(), ip, 9999);
        Serial.printf("Response from device '%s': %s\n", deviceName, response.c_str());
        return response.c_str();
    }
    else
    {
        String requestStr = "GET " + deviceName + " " + key;
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

String handleWhoMethod()
{
    if (!serverName.isEmpty())
    {
        String conf = "Server name: " + serverName + "\n";
        conf += "Server IP: " + serverIp.toString() + "\n";
        conf += "Broadcast IP: " + broadcastIP.toString() + "\n";
        conf += "Options " + optionsCallback() + "\n";
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
    const char *splitted = strtok((char *)packetData.c_str(), " ");
    bool device = true;
    char *tokens[4];
    int index = 0;
    while (splitted != NULL)
    {
        tokens[index] = const_cast<char *>(splitted);
        splitted = strtok(NULL, " ");
        index++;
    }

    if (index >= 4 && strcmp(strlwr(tokens[3]), "device") == 0)
    {
        device = true;
    }
    else
    {
        device = false;
    }

    String name = tokens[1];
    String ipString = tokens[2];
    Serial.printf("Name: '%s', IP: '%s'\n", name.c_str(), ipString.c_str());
    trim((char *)(name).c_str());
    if (name.length() > 0 && ipString.length() > 0)
    {
        if (name.equals(serverName) || validateDevice(name) != -1)
        {
            return "200 Name already in use";
        }

        IPAddress ip;
        if (ip.fromString(ipString))
        {

            char local[20];
            IPAddress ipLocal = serverIp;
            ipLocal.toString().toCharArray(local, 20);
            String response = "200 OK";
            if(!device){
                String requestStr = "ADD " + serverName + " " + local + " device";
                Serial.printf("Sending request to device '%s': %s\n", name.c_str(), requestStr.c_str());
                response = sendUDPRequest(requestStr.c_str(), ip, 9999);
            }
            if (response.startsWith("200"))
            {
                devices[numDevices].name = name;
                devices[numDevices].ip = ip;
                numDevices++;
                Serial.printf("New device added - Name: '%s', IP: '%s'\n", name.c_str(), ipString);
                return "200 OK " + name;
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

/**
 * @brief Manejador del metodo DEL
 * @param packetData Mensaje recibido
 * @return Respuesta del servidor
 * 
*/
String handleDELMethod(String packetData)
{
    String nameToRemove = packetData.substring(packetData.indexOf(' ', 1) + 1);

    for (int i = 0; i < numDevices; ++i)
    {
        if (devices[i].name == nameToRemove)
        {
            for (int j = i; j < numDevices - 1; ++j)
            {
                devices[j] = devices[j + 1];
            }
            numDevices--;

            Serial.printf("Device '%s' removed.\n", nameToRemove);
            return "200 OK %s", serverName;
        }
    }

    Serial.printf("Device '%s' not found.\n", nameToRemove);
    return "404 Not Found";
}

/**
 * @brief Manejador del metodo LIST
 * @return Lista de dispositivos
*/
String handleListMethod()
{
    String list = "Dispositivos:\n";
    for (int i = 0; i < numDevices; ++i)
    {
        list += devices[i].name + " " + devices[i].ip.toString() + "\n";
    }

    return list;
}

/**
 * @brief Procesa un paquete UDP
 * @param packetData Mensaje recibido
 * @return Respuesta del servidor
*/
String processUDPPacket(String packetData)
{
    //limpiar el mensaje
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
    }else if(packetData.startsWith("OPTIONS")){
        returnMessage = optionsCallback();
    }
    else if (packetData.startsWith("PING"))
    {
        // PING Responder con OK
        returnMessage = "200 OK";
    }
    else if (packetData.startsWith("ADD"))
    {
        // ADD Agregar un dispositivo
        if (numDevices == 16)
        {
            return "400 Max Devices Reached";
        }

        returnMessage = handleADDMethod(packetData);
    }
    else if (packetData.startsWith("DEL"))
    {
        // RM remover un dispositivo
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
