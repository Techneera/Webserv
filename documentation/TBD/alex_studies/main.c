//#include <sys/socket.h>
#include <arpa/inet.h> // helps ip addresses
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/* Sends an html file to the client */
void    send_html(int client_socket, const char *file_path)
{
    FILE    *html_file = fopen(file_path, "r");
    
    if (html_file == NULL)
    {
        perror("Could not open HTML file!\n");
        return;
    }

    char    buffer[BUFFER_SIZE];
    int     bytes_read;
    char    *http_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\
                            Connection: close\r\n\r\n";

    /* sends html to the browser socket, since its 0, and it behaves just like
     * a write function */
    send(client_socket, http_header, strlen(http_header), 0);

    /*
    for (size_t bytes_sent = 1; bytes_sent > 0; )
    {
        bytes_sent = send(client_socket, http_header, strlen(http_header), 0);
    }
    */

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, html_file)) > 0)
    {
        //send(client_socket, buffer, bytes_read, 0);
        for (int bytes_sent = 0, total_bytes = 0; total_bytes < bytes_read; )
        {
            bytes_sent = send(client_socket,
                              buffer + total_bytes,
                              bytes_read - total_bytes, 0);
            if (bytes_sent < 0)
            {
                perror("Sending Failed!\n");
                fclose(html_file);
                return ;
            }
            total_bytes += bytes_sent;
        }
    }
    printf("Finished sending html!\n");
    fclose(html_file);
}

int main(void)
{
    int                 server_socket, client_socket;
    struct sockaddr_in  server_addr, client_addr;
    socklen_t           client_len = sizeof(client_addr);
    char                request[BUFFER_SIZE];
    int                 received;

    signal(SIGPIPE, SIG_IGN);
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Failed to create socket!\n");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if ((bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
    {
        perror("Failed to bind socket address!\n");
        close(server_socket);
        exit(2);
    }

    if (listen(server_socket, 5) < 0)
    {
        perror("Could not listen from server!\n");
        exit(3);
    }

    while (1)
    {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0)
        {
            perror("Client connection attempt failed!\n");
            continue ;
        }
        printf("Client connected!\n");
        received = recv(client_socket, request, BUFFER_SIZE - 1, 0);
        if (received > 0)
        {
            request[received] = '\0';
            printf("\nREQUEST\n%s\n\n", request);
            send_html(client_socket, "index.html");
        }
        close(client_socket);
        printf("Client disconnected!\n");
    }

    close(server_socket);
    return (0);
}

