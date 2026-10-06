#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9410
#define MAX_BUFFER 4096

/*
 * Receive exactly one text line ending with '\n'.
 */
int recv_line(int sock_fd, char *buffer, size_t buffer_size)
{
    size_t index = 0;

    while (index < buffer_size - 1)
    {
        char character;

        ssize_t received = recv(sock_fd, &character, 1, 0);

        if (received == 0)
        {
            return 0;
        }

        if (received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        buffer[index++] = character;

        if (character == '\n')
        {
            break;
        }
    }

    buffer[index] = '\0';

    return 1;
}

/*
 * Send exactly 'length' bytes.
 * TCP send() may send fewer bytes than requested,
 * so this function continues until everything is sent.
 */
int send_all(int sock_fd, const void *buffer, size_t length)
{
    size_t total_sent = 0;
    const char *ptr = (const char *)buffer;

    while (total_sent < length)
    {
        ssize_t sent = send(sock_fd,
                            ptr + total_sent,
                            length - total_sent,
                            0);

        if (sent < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            return -1;
        }

        if (sent == 0)
        {
            return -1;
        }

        total_sent += (size_t)sent;
    }

    return 0;
}

/*
 * Send a local file to the Agent using the PUT protocol.
 *
 * The PUT header is sent first, followed immediately
 * by exactly the number of raw file bytes specified
 * in the header.
 */
int send_file(int sock_fd, const char *filename)
{
    struct stat file_info;

    if (stat(filename, &file_info) != 0)
    {
        printf("Local file not found: %s\n", filename);
        return -1;
    }

    if (file_info.st_size < 0)
    {
        printf("Invalid file size.\n");
        return -1;
    }

    unsigned long long filesize =
        (unsigned long long)file_info.st_size;

    FILE *file = fopen(filename, "rb");

    if (file == NULL)
    {
        printf("Unable to open file: %s\n", filename);
        return -1;
    }

    /*
     * Send the PUT command header.
     */
    char header[256];

    snprintf(header,
             sizeof(header),
             "PUT %s %llu\n",
             filename,
             filesize);

    if (send_all(sock_fd, header, strlen(header)) != 0)
    {
        fclose(file);
        return -1;
    }

    /*
     * Send the file contents in chunks.
     */
    unsigned char file_buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(file_buffer,
                               1,
                               sizeof(file_buffer),
                               file)) > 0)
    {
        if (send_all(sock_fd,
                     file_buffer,
                     bytes_read) != 0)
        {
            fclose(file);
            return -1;
        }
    }

    /*
     * Check whether the file ended normally.
     */
    if (ferror(file))
    {
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}

int main(void)
{
    int sock_fd;

    /*
     * Create TCP socket.
     */
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_fd < 0)
    {
        perror("socket");
        return EXIT_FAILURE;
    }

    /*
     * Configure Agent address.
     */
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET,
                  SERVER_IP,
                  &server_address.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    /*
     * Connect to the Agent.
     */
    if (connect(sock_fd,
                (struct sockaddr *)&server_address,
                sizeof(server_address)) < 0)
    {
        perror("connect");
        close(sock_fd);
        return EXIT_FAILURE;
    }

    printf("Connected to Agent at %s:%d\n",
           SERVER_IP,
           PORT);

    /*
     * Persistent command session.
     */
    char buffer[MAX_BUFFER];

    while (1)
    {
        printf("RemoteOps> ");
        fflush(stdout);

        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }

        /*
         * Handle PUT separately because PUT is followed
         * by raw file bytes rather than only a text command.
         */
        if (strncmp(buffer, "PUT ", 4) == 0)
        {
            char filename[128];
            unsigned long long declared_filesize;
            char extra_argument[128];

            int argument_count = sscanf(buffer + 4,
                                        "%127s %llu %127s",
                                        filename,
                                        &declared_filesize,
                                        extra_argument);

            /*
             * PUT must contain exactly:
             * PUT <filename> <filesize>
             */
            if (argument_count != 2)
            {
                printf("Usage: PUT <filename> <filesize>\n");
                continue;
            }

            /*
             * Check that the local file exists.
             */
            struct stat file_info;

            if (stat(filename, &file_info) != 0)
            {
                printf("Local file not found: %s\n",
                       filename);
                continue;
            }

            /*
             * Make sure the declared size matches
             * the actual local file size.
             */
            if ((unsigned long long)file_info.st_size
                != declared_filesize)
            {
                printf("File size mismatch. "
                       "Actual size: %lld bytes\n",
                       (long long)file_info.st_size);
                continue;
            }

            /*
             * Send PUT header followed by exactly
             * the file contents.
             */
            if (send_file(sock_fd, filename) != 0)
            {
                printf("File transfer failed.\n");
                break;
            }

            /*
             * Receive the Agent's response.
             */
            int result = recv_line(sock_fd,
                                   buffer,
                                   sizeof(buffer));

            if (result <= 0)
            {
                printf("Connection closed by Agent.\n");
                break;
            }

            printf("%s", buffer);

            continue;
        }

        /*
         * Normal text commands.
         *
         * send_all() is used instead of send() so that
         * the complete command is transmitted.
         */
        if (send_all(sock_fd,
                     buffer,
                     strlen(buffer)) != 0)
        {
            printf("Failed to send command.\n");
            break;
        }

        /*
         * Receive one complete response line.
         */
        int result = recv_line(sock_fd,
                               buffer,
                               sizeof(buffer));

        if (result == 0)
        {
            printf("Connection closed by Agent.\n");
            break;
        }

        if (result < 0)
        {
            perror("recv");
            break;
        }

        printf("%s", buffer);

        /*
         * QUIT ends the Controller session.
         */
        if (strncmp(buffer, "OK BYE", 6) == 0)
        {
            break;
        }
    }

    close(sock_fd);

    return EXIT_SUCCESS;
}
