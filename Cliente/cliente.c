#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

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

unsigned long getMillis()
{
    clock_t currentTime = clock();
    return (unsigned long)(currentTime * 1000 / CLOCKS_PER_SEC);
}


int main()
{
    int timeout = 10000;

    int clientSocket;
    struct sockaddr_in serverAddr;
    char message[MAX_MESSAGE_LEN];
    char buffer[MAX_MESSAGE_LEN];
    char serverIP[16]; // Se asume que una dirección IPv4 tiene un máximo de 15 caracteres.

    printf("Ingrese la dirección IP del servidor: \n");
    fgets(serverIP, sizeof(serverIP), stdin);
    trim(serverIP);

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
        memset(buffer, 0, sizeof(buffer));

        printf("Ingrese el mensaje (EXIT para salir): \n");
        fgets(message, MAX_MESSAGE_LEN, stdin);
        trim(message);


        if (strncmp(message, "EXIT", 4) == 0)
        {
            printf("Saliendo del cliente.\n");
            break;
        }

        // Envia mensaje al servidor
        sendto(clientSocket, (const char *)message, strlen(message) + 1,
               MSG_CONFIRM, (const struct sockaddr *)&serverAddr,
               sizeof(serverAddr));

        unsigned long startTime = getMillis();
        while (getMillis() - startTime < timeout)
        {
            ssize_t bytesRead = recvfrom(clientSocket, buffer, MAX_MESSAGE_LEN,
                                         MSG_WAITALL, NULL, NULL);

            if (bytesRead > 0)
            {
                // Respuesta del servidor recibida
                buffer[bytesRead] = '\0';
                printf("Respuesta del servidor: %s\n", buffer);
                break;
            }
        }


        if (getMillis() - startTime >= timeout)
        {
            printf("Timeout: No se recibió respuesta del servidor en %d segundos.\n", timeout / 1000);
            
        }
    }

    // Cerrar el socket
    close(clientSocket);

    return 0;
}
