#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9410

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
    int sock_fd;
    struct sockaddr_in server_addr;

    char command[1024];
    char response[1024];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        return 1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) < 0)
    {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Connected to RemoteOps Agent.\n");

    while (1)
    {
        printf("RemoteOps> ");
        fflush(stdout);

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        if (send(sock_fd, command, strlen(command), 0) < 0)
        {
            perror("send");
            break;
        }

        memset(response, 0, sizeof(response));

        int result = recv_line(sock_fd, response, sizeof(response));

        if (result == 0)
        {
            printf("Agent disconnected.\n");
            break;
        }

        if (result < 0)
        {
            perror("recv_line");
            break;
        }

        printf("%s", response);

        if (strcmp(command, "QUIT\n") == 0)
        {
            break;
        }
    }

    close(sock_fd);

    return 0;
}
