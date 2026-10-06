#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <time.h>
#include <pwd.h>

#define PORT 9410
#define AUTH_TOKEN "OPS-0486"
#define SID "6840"

#define MAX_BUFFER 4096
#define MAX_PROCESSES 20

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

int recv_exact(int sock_fd, void *buffer, size_t length)
{
    size_t total_received = 0;
    char *ptr = (char *)buffer;

    while (total_received < length)
    {
        ssize_t received = recv(sock_fd,
                                ptr + total_received,
                                length - total_received,
                                0);

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

        total_received += (size_t)received;
    }

    return 1;
}

int send_all(int sock_fd, const char *data, size_t length)
{
    size_t total_sent = 0;

    while (total_sent < length)
    {
        ssize_t bytes_sent = send(sock_fd,
                                  data + total_sent,
                                  length - total_sent,
                                  0);

        if (bytes_sent < 0)
        {
            return -1;
        }

        total_sent += bytes_sent;
    }

    return 0;
}

int send_response(int sock_fd, const char *response)
{
    return send_all(sock_fd, response, strlen(response));
}

int get_sysinfo(char *output, size_t output_size)
{
    struct sysinfo info;

    if (sysinfo(&info) != 0)
    {
        return -1;
    }

    FILE *load_file = fopen("/proc/loadavg", "r");

    if (load_file == NULL)
    {
        return -1;
    }

    double cpu_load;

    if (fscanf(load_file, "%lf", &cpu_load) != 1)
    {
        fclose(load_file);
        return -1;
    }

    fclose(load_file);

    unsigned long long total_memory =
        (unsigned long long)info.totalram * info.mem_unit;

    unsigned long long free_memory =
        (unsigned long long)info.freeram * info.mem_unit;

    unsigned long long used_memory =
        total_memory - free_memory;

    unsigned long long used_memory_mb =
        used_memory / (1024ULL * 1024ULL);

    snprintf(output,
             output_size,
             "OK SYSINFO %.2f %llu %ld SID:%s\n",
             cpu_load,
             used_memory_mb,
             info.uptime,
             SID);

    return 0;
}

int get_process_list(char *output, size_t output_size)
{
    FILE *process_file = popen("ps -eo pid=,comm=", "r");

    if (process_file == NULL)
    {
        return -1;
    }

    char line[256];
    size_t used = 0;
    int process_count = 0;

    used += snprintf(output + used,
                     output_size - used,
                     "OK PROCS ");

    while (fgets(line, sizeof(line), process_file) != NULL &&
           process_count < MAX_PROCESSES)
    {
        int pid;
        char process_name[128];

        if (sscanf(line, "%d %127s", &pid, process_name) != 2)
        {
            continue;
        }

        int written;

        if (process_count > 0)
        {
            written = snprintf(output + used,
                               output_size - used,
                               ",");
            used += written;
        }

        written = snprintf(output + used,
                           output_size - used,
                           "%s/%d",
                           process_name,
                           pid);

        used += written;
        process_count++;

        if (used >= output_size - 100)
        {
            break;
        }
    }

    pclose(process_file);

    snprintf(output + used,
             output_size - used,
             " SID:%s\n",
             SID);

    return 0;
}

int execute_command(const char *command,
                    char *output,
                    size_t output_size)
{
    if (strcmp(command, "DATE") == 0)
    {
        time_t current_time = time(NULL);
        struct tm local_time;

        if (localtime_r(&current_time, &local_time) == NULL)
        {
            return -1;
        }

        strftime(output,
                 output_size,
                 "%Y-%m-%d %H:%M:%S",
                 &local_time);

        return 0;
    }

    if (strcmp(command, "UPTIME") == 0)
    {
        struct sysinfo info;

        if (sysinfo(&info) != 0)
        {
            return -1;
        }

        snprintf(output,
                 output_size,
                 "%ld seconds",
                 info.uptime);

        return 0;
    }

    if (strcmp(command, "DISKFREE") == 0)
    {
        struct statvfs disk_info;

        if (statvfs(".", &disk_info) != 0)
        {
            return -1;
        }

        unsigned long long free_bytes =
            (unsigned long long)disk_info.f_bavail *
            disk_info.f_frsize;

        unsigned long long free_mb =
            free_bytes / (1024ULL * 1024ULL);

        snprintf(output,
                 output_size,
                 "%llu MB",
                 free_mb);

        return 0;
    }

    if (strcmp(command, "HOSTNAME") == 0)
    {
        char hostname[256];

        if (gethostname(hostname, sizeof(hostname)) != 0)
        {
            return -1;
        }

        hostname[sizeof(hostname) - 1] = '\0';

        snprintf(output,
                 output_size,
                 "%s",
                 hostname);

        return 0;
    }

    if (strcmp(command, "WHOAMI") == 0)
    {
        struct passwd password_entry;
        struct passwd *result = NULL;
        char password_buffer[1024];

        if (getpwuid_r(geteuid(),
                       &password_entry,
                       password_buffer,
                       sizeof(password_buffer),
                       &result) != 0 ||
            result == NULL)
        {
            return -1;
        }

        snprintf(output,
                 output_size,
                 "%s",
                 password_entry.pw_name);

        return 0;
    }

    return 1;
}

int main(void)
{
    int server_fd;
    int client_fd;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    char buffer[MAX_BUFFER];
    char response[MAX_BUFFER];
    char command_output[2048];

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

    if (bind(server_fd,
             (struct sockaddr *)&server_addr,
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
        int result = recv_line(client_fd,
                               buffer,
                               sizeof(buffer));

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
    if (strcmp(buffer, "AUTH " AUTH_TOKEN "\n") == 0)
    {
        snprintf(response,
                 sizeof(response),
                 "OK AUTHENTICATED SID:%s\n",
                 SID);

        send_response(client_fd, response);

        authenticated = 1;

        printf("Controller authenticated successfully.\n");
    }
    else if (strncmp(buffer, "AUTH ", 5) == 0)
    {
        snprintf(response,
                 sizeof(response),
                 "ERR 001 AUTH_FAILED SID:%s\n",
                 SID);

        send_response(client_fd, response);

        printf("Authentication failed.\n");
    }
    else
    {
        snprintf(response,
                 sizeof(response),
                 "ERR 003 AUTH_REQUIRED SID:%s\n",
                 SID);

        send_response(client_fd, response);

        printf("Command rejected before authentication.\n");
    }

    continue;
}
        if (strcmp(buffer, "SYSINFO\n") == 0)
        {
            if (get_sysinfo(response, sizeof(response)) == 0)
            {
                send_response(client_fd, response);
            }
            else
            {
                snprintf(response,
                         sizeof(response),
                         "ERR 006 SYSINFO_FAILED SID:%s\n",
                         SID);

                send_response(client_fd, response);
            }

            continue;
        }

        if (strcmp(buffer, "LISTPROC\n") == 0)
        {
            if (get_process_list(response, sizeof(response)) == 0)
            {
                send_response(client_fd, response);
            }
            else
            {
                snprintf(response,
                         sizeof(response),
                         "ERR 007 PROCESS_LIST_FAILED SID:%s\n",
                         SID);

                send_response(client_fd, response);
            }

            continue;
        }

	if (strncmp(buffer, "EXEC ", 5) == 0)
{
    char exec_name[128];
    char extra_argument[128];

    int argument_count = sscanf(buffer + 5,
                                "%127s %127s",
                                exec_name,
                                extra_argument);

    if (argument_count != 1)
    {
        snprintf(response,
                 sizeof(response),
                 "ERR 002 COMMAND_NOT_ALLOWED SID:%s\n",
                 SID);

        send_response(client_fd, response);
        continue;
    }

    int exec_result = execute_command(exec_name,
                                      command_output,
                                      sizeof(command_output));

    if (exec_result == 1)
    {
        snprintf(response,
                 sizeof(response),
                 "ERR 002 COMMAND_NOT_ALLOWED SID:%s\n",
                 SID);

        send_response(client_fd, response);
    }
    else if (exec_result < 0)
    {
        snprintf(response,
                 sizeof(response),
                 "ERR 008 EXEC_FAILED SID:%s\n",
                 SID);

        send_response(client_fd, response);
    }
    else
    {
        snprintf(response,
                 sizeof(response),
                 "OK EXEC_RESULT %s SID:%s\n",
                 command_output,
                 SID);

        send_response(client_fd, response);
    }

    continue;
}

if (strncmp(buffer, "PUT ", 4) == 0)
{
    char filename[128];
    char extra_argument[128];
    unsigned long long filesize;

    int argument_count = sscanf(buffer + 4,
                                "%127s %llu %127s",
                                filename,
                                &filesize,
                                extra_argument);

    if (argument_count != 2)
    {
        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    /*
     * Simple filename validation.
     * Uploaded files must stay inside the personalised
     * agentfiles/IT24100486 directory.
     */
    if (filename[0] == '\0' ||
        strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0 ||
        strchr(filename, '/') != NULL ||
        strchr(filename, '\\') != NULL)
    {
        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    /*
     * Implementation safety limit.
     * The assignment requires FILE_TOO_LARGE handling but
     * does not specify a numeric maximum, so 100 MiB is
     * used as an implementation decision.
     */
    const unsigned long long MAX_FILE_SIZE =
        100ULL * 1024ULL * 1024ULL;

    if (filesize > MAX_FILE_SIZE)
    {
        send_response(client_fd,
                      "ERR 004 FILE_TOO_LARGE SID:" SID "\n");
        continue;
    }

    char filepath[512];

    snprintf(filepath,
             sizeof(filepath),
             "./agentfiles/IT24100486/%s",
             filename);

    FILE *file = fopen(filepath, "wb");

    if (file == NULL)
    {
        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    unsigned char file_buffer[4096];
    unsigned long long remaining = filesize;
    int transfer_success = 1;

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

        int result = recv_exact(client_fd,
                                file_buffer,
                                chunk_size);

        if (result != 1)
        {
            transfer_success = 0;
            break;
        }

        size_t written = fwrite(file_buffer,
                                1,
                                chunk_size,
                                file);

        if (written != chunk_size)
        {
            transfer_success = 0;
            break;
        }

        remaining -= chunk_size;
    }

    fclose(file);

    if (!transfer_success)
    {
        remove(filepath);
        break;
    }

    char response[256];

    snprintf(response,
             sizeof(response),
             "OK FILE_RECEIVED %s SID:%s\n",
             filename,
             SID);

    send_response(client_fd, response);
    continue;
}

/*
 * GET <filename>
 *
 * Send the requested file from the Agent to the Controller.
 * The response header is sent first, followed immediately
 * by exactly the number of raw bytes specified in the header.
 */
if (strncmp(buffer, "GET ", 4) == 0)
{
    char filename[128];
    char extra_argument[128];

    int argument_count = sscanf(buffer + 4,
                                "%127s %127s",
                                filename,
                                extra_argument);

    if (argument_count != 1)
    {
        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    /*
     * Validate filename so the requested file stays
     * inside the personalised agent storage directory.
     */
    if (filename[0] == '\0' ||
        strcmp(filename, ".") == 0 ||
        strcmp(filename, "..") == 0 ||
        strchr(filename, '/') != NULL ||
        strchr(filename, '\\') != NULL)
    {
        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    char filepath[512];

    snprintf(filepath,
             sizeof(filepath),
             "./agentfiles/IT24100486/%s",
             filename);

    FILE *file = fopen(filepath, "rb");

    if (file == NULL)
    {
        send_response(client_fd,
                      "ERR 005 FILE_NOT_FOUND SID:" SID "\n");
        continue;
    }

    /*
     * Determine the exact file size before sending
     * the response header.
     */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);

        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    long file_size = ftell(file);

    if (file_size < 0)
    {
        fclose(file);

        send_response(client_fd,
                      "ERR 010 INVALID_FILE_REQUEST SID:" SID "\n");
        continue;
    }

    rewind(file);

    char response[256];

    snprintf(response,
             sizeof(response),
             "OK FILE_SEND %s %ld SID:%s\n",
             filename,
             file_size,
             SID);

    if (send_response(client_fd, response) != 0)
    {
        fclose(file);
        break;
    }

    /*
     * Send exactly the file contents after the header.
     * TCP may split the data into multiple packets, so
     * send_all() is used for every chunk.
     */
    unsigned char file_buffer[4096];
    size_t bytes_read;
    int send_success = 1;

    while ((bytes_read = fread(file_buffer,
                               1,
                               sizeof(file_buffer),
                               file)) > 0)
    {
        if (send_all(client_fd,
                    (const char *)file_buffer,
                     bytes_read) != 0)
        {
            send_success = 0;
            break;
        }
    }

    int read_error = ferror(file);

    fclose(file);

    if (!send_success || read_error)
    {
        break;
    }

    continue;
} 

       if (strcmp(buffer, "QUIT\n") == 0)
        {
            snprintf(response,
                     sizeof(response),
                     "OK BYE SID:%s\n",
                     SID);

            send_response(client_fd, response);

            printf("Controller requested disconnect.\n");
            break;
        }

        snprintf(response,
                 sizeof(response),
                 "ERR 009 UNKNOWN_COMMAND SID:%s\n",
                 SID);

        send_response(client_fd, response);
    }

    close(client_fd);
    close(server_fd);

    return 0;
}
