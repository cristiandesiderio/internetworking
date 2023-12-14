#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#include <errno.h>

#define SERVER_PORT 9999
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
    char serverIP[16];

    // Crear un socket UDP
    if ((clientSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    printf("Ingrese la dirección IP del servidor: \n");
    fgets(serverIP, sizeof(serverIP), stdin);
    trim(serverIP);

    serverAddr.sin_addr.s_addr = inet_addr(serverIP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0)
    {
        perror("Error al configurar la dirección del servidor\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }


    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    int ret = setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if (ret < 0)
    {
        perror("Error al configurar el timeout\n");
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

        int bytes_received = recvfrom(clientSocket, buffer, MAX_MESSAGE_LEN, MSG_WAITALL, NULL, NULL);

        // Si no se recibieron bytes el servidor
        if (bytes_received < 0)
        {
            // Timeout
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                printf("El timeout se activó.\n");
            }
            else
            {
                perror("Error al recibir la respuesta del servidor\n");
            }
        }
        else
        {
            buffer[bytes_received] = '\0';
            printf("Respuesta del servidor: %s\n", buffer);
        }


    }


    close(clientSocket);

    return 0;
}
