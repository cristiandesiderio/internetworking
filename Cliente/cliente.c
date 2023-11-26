#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>

#define SERVER_PORT 9999 // Cambia esto con el puerto de tu servidor
#define MAX_MESSAGE_LEN 1024

void trim(char *str)
{
    while (isspace(*str) || *str == '\n' || *str == '\r')
    {
        str++;
    }
    size_t len = strlen(str);
    while (len > 0 && (isspace(str[len - 1]) || str[len - 1] == '\n' || str[len - 1] == '\r'))
    {
        len--;
    }
    str[len] = '\0';
}

void flush_stdin()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

int main()
{
    int clientSocket;
    struct sockaddr_in serverAddr;
    char message[MAX_MESSAGE_LEN];
    char buffer[MAX_MESSAGE_LEN];
    char serverIP[16]; // Se asume que una dirección IPv4 tiene un máximo de 15 caracteres.

    // Leer la dirección IP desde la consola
    printf("Ingrese la dirección IP del servidor: ");
    scanf("%15s", serverIP);

    // Crear un socket UDP
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    // Configurar la información del servidor
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0)
    {
        perror("Error al configurar la dirección del servidor");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    flush_stdin();

    while (1)
    {
        // Obtener el mensaje del usuario
        printf("Ingrese el mensaje (EXIT para salir): ");
        fgets(message, MAX_MESSAGE_LEN, stdin);
        trim(message);
        // Enviar el mensaje al servidor
        sendto(clientSocket, (const char *)message, strlen(message) + 1,
               MSG_CONFIRM, (const struct sockaddr *)&serverAddr,
               sizeof(serverAddr));

        // Salir si el usuario escribe "EXIT"
        if (strncmp(message, "EXIT", 4) == 0)
        {
            printf("Saliendo del cliente.\n");
            break;
        }

        flush_stdin();
        // Esperar la respuesta del servidor
        ssize_t bytesRead = recvfrom(clientSocket, buffer, MAX_MESSAGE_LEN,
                                     MSG_WAITALL, NULL, NULL);
        if (bytesRead == -1)
        {
            perror("Error al recibir la respuesta del servidor");
            close(clientSocket);
            exit(EXIT_FAILURE);
        }

        // Agregar el terminador nulo al final del buffer
        buffer[bytesRead] = '\0';
        printf("Respuesta del servidor: %s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }

    // Cerrar el socket
    close(clientSocket);

    return 0;
}
