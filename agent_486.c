#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9410
#define AUTH_TOKEN "OPS-0486"
#define SID "6840"

int recv_line(int sock_fd, char *buffer, size_t buffer_size)
{
    size_t i = 0;

    while (i < buffer_size - 1)
    {
        char ch;
        ssize_t bytes_received = recv(sock_fd, &ch, 1, 0);

        if (bytes_received == 0)
        {
            return 0;
        }

        if (bytes_received < 0)
        {
            return -1;
        }

        buffer[i++] = ch;

        if (ch == '\n')
        {
            break;
        }
    }

    buffer[i] = '\0';

    return 1;
}

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("RemoteOps Agent listening on TCP port %d...\n", PORT);

    client_fd = accept(server_fd,
                       (struct sockaddr *)&client_addr,
                       &client_len);

    if (client_fd < 0)
    {
        perror("accept");
        close(server_fd);
        return 1;
    }

    printf("Controller connected successfully.\n");

    int authenticated = 0;

    while (1)
    {
        int result = recv_line(client_fd, buffer, sizeof(buffer));

        if (result == 0)
        {
            printf("Controller disconnected.\n");
            break;
        }

        if (result < 0)
        {
            perror("recv_line");
            break;
        }

        if (!authenticated)
        {
            if (strcmp(buffer, "AUTH OPS-0486\n") == 0)
            {
                const char *response =
                    "OK AUTHENTICATED SID:6840\n";

                send(client_fd, response, strlen(response), 0);

                authenticated = 1;

                printf("Controller authenticated successfully.\n");
            }
            else
            {
                const char *response =
                    "ERR 001 AUTH_FAILED SID:6840\n";

                send(client_fd, response, strlen(response), 0);

                printf("Authentication failed.\n");

                break;
            }
        }
        else
        {
            printf("Received command: %s", buffer);

            const char *response =
                "ERR 003 AUTH_REQUIRED SID:6840\n";

            send(client_fd, response, strlen(response), 0);
        }
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
