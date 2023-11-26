#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define MAX_DEVICES 100
#define MAX_NAME_LENGTH 50

// Estructura para representar un dispositivo en la red
struct Device {
    char name[MAX_NAME_LENGTH];
    char ip[INET_ADDRSTRLEN];
};

struct Device devices[MAX_DEVICES];
int numDevices = 0;

// Función para buscar un dispositivo por nombre en la red (búsqueda en grafo)
int findDevice(const char* name) {
    for (int i = 0; i < numDevices; i++) {
        if (strcmp(devices[i].name, name) == 0) {
            return i; // Se encontró el dispositivo
        }
    }
    return -1; // Dispositivo no encontrado
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("Error al crear el socket");
        exit(1);
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Vincular el socket al puerto
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error al vincular el socket");
        exit(1);
    }

    printf("Esperando mensajes...\n");

    while (1) {
        socklen_t len = sizeof(client_addr);

        // Recibir un mensaje
        ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &len);
        if (recv_len == -1) {
            perror("Error al recibir mensaje");
            continue;
        }

        // Procesar el mensaje
        buffer[recv_len] = '\0';
        printf("Mensaje: %s\n", buffer);
        if (strncmp(buffer, "SET", 3) == 0) {
            // Mensaje SET: Almacena el nombre del dispositivo
            char* name = strtok(buffer + 4, " ");
            if (name != NULL) {
                // Almacenar el nombre del dispositivo en la lista
                strncpy(devices[numDevices].name, name, MAX_NAME_LENGTH);
                inet_ntop(AF_INET, &(client_addr.sin_addr), devices[numDevices].ip, INET_ADDRSTRLEN);
                numDevices++;
                printf("Nombre del dispositivo '%s' almacenado.\n", name);
                sendto(sockfd, "200 OK", 6, 0, (struct sockaddr*)&client_addr, len);
            }
        } else if (strncmp(buffer, "TEMP", 4) == 0 || strncmp(buffer, "AIR", 3) == 0) {
            // Mensaje TEMP o AIR: Buscar el dispositivo y responder o 404 si no se encuentra
            char* deviceName = strtok(buffer + 5, " ");
            if (deviceName != NULL) {
                int deviceIndex = findDevice(deviceName);
                if (deviceIndex != -1) {
                    // Dispositivo encontrado, generar respuesta (simulada)
                    char response[100];
                    snprintf(response, sizeof(response), "%s para %s: %d", buffer, devices[deviceIndex].name, rand() % 41);
                    sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, len);
                } else {
                    // Dispositivo no encontrado, enviar 404
                    sendto(sockfd, "404 Dispositivo no encontrado", 29, 0, (struct sockaddr*)&client_addr, len);
                }
            }
        } else if (strncmp(buffer, "ADD", 3) == 0) {
            // Mensaje ADD: Agregar manualmente un dispositivo conocido
            char* deviceInfo = strtok(buffer + 4, " ");
            if (deviceInfo != NULL) {
                // Almacenar la información del dispositivo manualmente
                strncpy(devices[numDevices].name, deviceInfo, MAX_NAME_LENGTH);
                inet_ntop(AF_INET, &(client_addr.sin_addr), devices[numDevices].ip, INET_ADDRSTRLEN);
                numDevices++;
                printf("Dispositivo agregado manualmente: %s\n", deviceInfo);
            }
        } else if (strncmp(buffer, "DEL", 3) == 0) {
            // Mensaje DEL: Eliminar manualmente un dispositivo conocido
            char* deviceName = strtok(buffer + 4, " ");
            printf(deviceName);
            if (deviceName != NULL) {
                int deviceIndex = findDevice(deviceName);
                if (deviceIndex != -1) {
                    // Eliminar el dispositivo de la lista
                    memmove(&devices[deviceIndex], &devices[deviceIndex + 1], (numDevices - deviceIndex - 1) * sizeof(struct Device));
                    numDevices--;
                    printf("Dispositivo eliminado manualmente: %s\n", deviceName);
                }
            }
        } else if (strncmp(buffer, "WHO", 3) == 0) {
            // Mensaje WHO: Responder con la IP y el nombre del dispositivo
            char response[100];
            printf(buffer + 4);
            snprintf(response, sizeof(response), "IP: %s, Nombre: %s", inet_ntoa(client_addr.sin_addr), devices[findDevice(buffer + 4)].name);
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, len);
        } else if (strncmp(buffer, "LIST", 4) == 0) {
            // Mensaje LIST: Responder con la lista de dispositivos conocidos
            char response[1024];
            response[0] = '\0';
            for (int i = 0; i < numDevices; i++) {
                strcat(response, devices[i].name);
                strcat(response, "\n");
            }
            printf("hola");
            sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, len);
        } else {
            
            printf("Mensaje no reconocido: %s\n", buffer);
            sendto(sockfd, "400 Bad Request", 15, 0, (struct sockaddr*)&client_addr, len);
        }
    }

    // Cerrar el socket
    close(sockfd);

    return 0;
}
