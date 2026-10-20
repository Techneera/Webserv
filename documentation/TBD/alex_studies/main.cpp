//#include <sys/socket.h>
#include <arpa/inet.h> // helps ip addresses
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 1024

int PORT = 8000;
volatile sig_atomic_t   interrupt = 0;

void signalHandler_Ctrl_C(int sig)
{
    (void)sig;
    std::cerr << "CTRL-C called\n";
}
void signalHandler(int signalCode)
{
    std::cerr << "Server Interrupted by SIGINT!\n";
    interrupt = signalCode;
}

/* Sends an html file to the client */
void    send_file(int client_socket, const char *file_path, std::string header)
{
    FILE    *file = fopen(file_path, "r");
    
    if (file == NULL)
    {
        perror("Could not open HTML file!\n");
        return;
    }

    char        buffer[BUFFER_SIZE];
    int         bytes_read;

    /* sends html to the browser socket, since its 0, and it behaves just like
     * a write function */
    send(client_socket, header.c_str(), header.size(), 0);

    /*
    for (size_t bytes_sent = 1; bytes_sent > 0; )
    {
        bytes_sent = send(client_socket, http_header, strlen(http_header), 0);
    }
    */

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
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
                fclose(file);
                return ;
            }
            total_bytes += bytes_sent;
        }
    }
    std::cout << "Finished sending " << file_path << "!\n";
    fclose(file);
}

int main(void)
{
    int                 server_socket, client_socket;
    struct sockaddr_in  server_addr, client_addr;
    socklen_t           client_len = sizeof(client_addr);
    char                request[BUFFER_SIZE];
    int                 received;

    /* Handle the SIGINT */
    struct sigaction    newAction;

    newAction.sa_handler = signalHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;
    sigaction(SIGINT, &newAction, NULL);

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Failed to create socket!\n");
        exit(1);
    }

    int i = 0;
    int ret;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    ret = bind(server_socket,
            (struct sockaddr *)&server_addr, sizeof(server_addr));
    while (ret < 0 && i < 10)
    {
        server_addr.sin_port = htons(PORT);
        ret = bind(server_socket,
                (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (i == 9)
        {
            std::cerr << "Failed to bind socket address to port "
                      << PORT
                      << "! Closing program \n";
            close(server_socket);
            exit(2);
        }
        if (ret < 0)
        {
            std::cerr << "Failed to bind socket address to port "
                      << PORT << "\n";
            i++;
            PORT += 60;
        }
    }

    std::cout << "Opened Port " << PORT << "\n";

    if (listen(server_socket, 5) < 0)
    {
        perror("Could not listen from server!\n");
        exit(3);
    }

    for (size_t i = 0; !interrupt; i++)
    {
        char    css[] = "webpage/index.css";
        char    html[] = "index.html";
        char    *file;
        std::string http_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\
                            Connection: close\r\n\r\n";
        std::string css_header = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\
                            Connection: close\r\n\r\n";
        std::string header;


        if (i % 2 == 0)
        {
            file = html;
            header = http_header;
        }
        else
        {
            file = css;
            header = css_header;
        }
        client_socket = accept(server_socket,
                (struct sockaddr*)&client_addr, &client_len);
        /*if (fcntl(client_socket, F_SETFL,
                fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK))
            exit(1);*/

        if (client_socket < 0)
        {
            std::cout << "Client connection attempt failed!\n";
            continue ;
        }
        std::cerr << "Client connected!\n";
        received = recv(client_socket, request, BUFFER_SIZE - 1, 0);
        if (received > 0)
        {
            request[received] = '\0';
            std::cout << "\nREQUEST\n" << request << "\n\n";
            send_file(client_socket, file, header);
        }
        close(client_socket);
        std::cout << "Client disconnected!\n";
    }

    close(server_socket);
    return (0);
}

