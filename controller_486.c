#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdint.h>

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

/* Calculate elapsed transfer time in seconds. */
double elapsed_seconds(struct timeval start, struct timeval end)
{
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_usec - start.tv_usec) / 1000000.0;
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

typedef struct
{
    int udp_socket;
    volatile int active;
} monitor_context;

void *monitor_receiver(void *arg)
{
    monitor_context *context = (monitor_context *)arg;

    char buffer[256];

    while (context->active)
    {
        ssize_t received = recvfrom(context->udp_socket,
                                    buffer,
                                    sizeof(buffer) - 1,
                                    0,
                                    NULL,
                                    NULL);

        if (received < 0)
        {
            if (!context->active)
                break;

            continue;
        }

        buffer[received] = '\0';

        printf("\n[UDP] %s", buffer);
        printf("RemoteOps> ");
        fflush(stdout);
    }

    return NULL;
}

int main(void)
{
    int sock_fd;

    int udp_socket;
    struct sockaddr_in udp_addr;
    monitor_context monitor;
    pthread_t monitor_thread;
    int monitoring = 0;

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
struct timeval transfer_start;
struct timeval transfer_end;

gettimeofday(&transfer_start, NULL);

if (send_file(sock_fd, filename) != 0)
{
    printf("File transfer failed.\n");
    break;
}

gettimeofday(&transfer_end, NULL);

double transfer_time =
    elapsed_seconds(transfer_start, transfer_end);

double throughput = 0.0;

if (transfer_time > 0.0)
{
    throughput =
        (double)declared_filesize / transfer_time;
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

	    printf("PUT throughput: %.2f bytes/sec\n", throughput);

            continue;
        }

        /*
         * Handle GET separately because GET is followed
         * by raw file bytes from the Agent.
         */
        if (strncmp(buffer, "GET ", 4) == 0)
        {
            char filename[128];
            char extra_argument[128];

            int argument_count = sscanf(buffer + 4,
                                        "%127s %127s",
                                        filename,
                                        extra_argument);

            /*
             * GET must contain exactly:
             * GET <filename>
             */
            if (argument_count != 1)
            {
                printf("Usage: GET <filename>\n");
                continue;
            }

            /*
             * Send the GET command to the Agent.
             */
            if (send_all(sock_fd,
                         buffer,
                         strlen(buffer)) != 0)
            {
                printf("Failed to send GET command.\n");
                break;
            }

            /*
             * Receive the Agent response header.
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

            /*
             * Check whether the requested file exists.
             */
            if (strncmp(buffer, "ERR 005 FILE_NOT_FOUND", 22) == 0)
            {
                printf("%s", buffer);
                continue;
            }

            /*
             * Parse:
             * OK FILE_SEND <filename> <filesize> SID:6840
             */
            char response_filename[128];
            unsigned long long filesize;
            char sid[32];

            int parsed = sscanf(buffer,
                                "OK FILE_SEND %127s %llu SID:%31s",
                                response_filename,
                                &filesize,
                                sid);

            if (parsed != 3)
            {
                printf("%s", buffer);
                continue;
            }

            printf("%s", buffer);

            /*
             * Save the received file locally.
             */
            FILE *file = fopen(response_filename, "wb");

            if (file == NULL)
            {
                perror("fopen");
                break;
            }

	    struct timeval transfer_start;
	    struct timeval transfer_end;

	    gettimeofday(&transfer_start, NULL);

            unsigned char file_buffer[4096];
            unsigned long long remaining = filesize;
            int transfer_success = 1;

            /*
             * Receive exactly the number of bytes
             * specified by the Agent.
             */
            while (remaining > 0)
            {
                size_t chunk_size;

                if (remaining > sizeof(file_buffer))
                {
                    chunk_size = sizeof(file_buffer);
                }
                else
                {
                    chunk_size = (size_t)remaining;
                }

                size_t total_received = 0;

                while (total_received < chunk_size)
                {
                    ssize_t received = recv(sock_fd,
                                            file_buffer + total_received,
                                            chunk_size - total_received,
                                            0);

                    if (received < 0)
                    {
                        if (errno == EINTR)
                        {
                            continue;
                        }

                        perror("recv");
                        transfer_success = 0;
                        break;
                    }

                    if (received == 0)
                    {
                        printf("Connection closed during file transfer.\n");
                        transfer_success = 0;
                        break;
                    }

                    total_received += (size_t)received;
                }

                if (!transfer_success)
                {
                    break;
                }

                size_t written = fwrite(file_buffer,
                                        1,
                                        chunk_size,
                                        file);

                if (written != chunk_size)
                {
                    perror("fwrite");
                    transfer_success = 0;
                    break;
                }

                remaining -= chunk_size;
            }

            fclose(file);

if (!transfer_success)
{
    remove(response_filename);
    break;
}

gettimeofday(&transfer_end, NULL);

double transfer_time =
    elapsed_seconds(transfer_start, transfer_end);

double throughput = 0.0;

if (transfer_time > 0.0)
{
    throughput =
        (double)filesize / transfer_time;
}

printf("File received successfully: %s (%llu bytes)\n",
       response_filename,
       filesize);

printf("GET throughput: %.2f bytes/sec\n",
       throughput);

            continue;
        }

        /*
         * Handle MONITOR START separately because
         * the Controller must prepare a UDP socket
         * before asking the Agent to start monitoring.
         */
        if (strncmp(buffer, "MONITOR START ", 14) == 0)
        {
            int udp_port;

            if (sscanf(buffer + 14, "%d", &udp_port) != 1 ||
                udp_port < 1 || udp_port > 65535)
            {
                printf("Usage: MONITOR START <udp_port>\n");
                continue;
            }

            udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

            if (udp_socket < 0)
            {
                perror("UDP socket");
                continue;
            }

            memset(&udp_addr, 0, sizeof(udp_addr));

            udp_addr.sin_family = AF_INET;
            udp_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            udp_addr.sin_port = htons((uint16_t)udp_port);

            if (bind(udp_socket,
                     (struct sockaddr *)&udp_addr,
                     sizeof(udp_addr)) < 0)
            {
                perror("UDP bind");
                close(udp_socket);
                continue;
            }

	    struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            setsockopt(udp_socket,
                       SOL_SOCKET,
                       SO_RCVTIMEO,
                       &timeout,
                       sizeof(timeout));

            monitor.udp_socket = udp_socket;
            monitor.active = 1;

            if (pthread_create(&monitor_thread,
                               NULL,
                               monitor_receiver,
                               &monitor) != 0)
            {
                perror("pthread_create");
                monitor.active = 0;
                close(udp_socket);
                continue;
            }

	    monitoring = 1;

            if (send_all(sock_fd,
                         buffer,
                         strlen(buffer)) != 0)
            {
                printf("Failed to send MONITOR START command.\n");
                monitor.active = 0;
                close(udp_socket);
                pthread_join(monitor_thread, NULL);
                break;
            }

            int result = recv_line(sock_fd,
                                   buffer,
                                   sizeof(buffer));

            if (result <= 0)
            {
                printf("Connection closed by Agent.\n");
                monitor.active = 0;
                close(udp_socket);
                pthread_join(monitor_thread, NULL);
                break;
            }

            printf("%s", buffer);

            continue;
        }

        if (strcmp(buffer, "MONITOR STOP\n") == 0)
        {
            /*
             * Tell the Agent to stop monitoring first.
             */

	    printf("STOP BLOCK ENTERED\n");
	    fflush(stdout);

            if (send_all(sock_fd,
                         buffer,
                         strlen(buffer)) != 0)
            {
                printf("Failed to send MONITOR STOP command.\n");
                break;
            }

            int result = recv_line(sock_fd,
                                   buffer,
                                   sizeof(buffer));

            if (result <= 0)
            {
                printf("Connection closed by Agent.\n");
                break;
            }

            printf("%s", buffer);

            /*
             * Now stop the local UDP receiver.
             */
            if (monitoring)
            {
                monitor.active = 0;
                pthread_join(monitor_thread, NULL);
                close(udp_socket);
                monitoring = 0;
            }

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
