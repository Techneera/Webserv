#include <sys/socket.h>
#include <arpa/inet.h> // helps ip addresses
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <vector>

typedef int fd;

typedef std::vector<fd> fdVec;
typedef fdVec::iterator fdVecIt;

#define BUFFER_SIZE 1024
#define MAX_EVENTS 1024

int PORT = 8000;
volatile sig_atomic_t   interrupt = 0;
char        html_home[] = "index.html";
char        css_home[] = "webpage/index.css";
char        js_home[] = "webpage/index.js";
char        http_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\
                             Connection: close\r\n\r\n";
char        css_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\
                            Connection: close\r\n\r\n";
char        js_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/js\r\n\
                            Connection: close\r\n\r\n";

void respond(int fd, char *request, char *header, char *file);

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
        close(server_socket);
        exit(3);
    }

    int                 n;
    int                 nfds;
    int                 epollFd = epoll_create1(0);
    struct epoll_event  eEvent;
    struct epoll_event  eEvents[MAX_EVENTS];
    char                *file = NULL;
    char                *header = NULL;

    eEvent.events = EPOLLIN;
    eEvent.data.fd = server_socket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, server_socket, & eEvent) == -1)
    {
        close(server_socket);
        // TODO: Close all epoll fds needed
        std::cerr << "ERROR: Failed to add epoll\n";
        exit(1);
    }
    for (;!interrupt;)
    {
        nfds = epoll_wait(epollFd, eEvents, MAX_EVENTS, -1);
        std::cout << "epoll_wait return: " <<  nfds << "\n";
        if (nfds == -1)
        {
            std::cerr << "ERROR: epoll_wait failed!\n";
            // TODO: close all epoll fds needed
            close(server_socket);
            close(epollFd);
            exit(1);
        }
        for (n = 0; n < nfds; n++)
        {
            std::cout << "size of eEvents: " << n << "\n";
            if (eEvents[n].data.fd == server_socket)
            {
                client_socket = accept(server_socket,
                        (struct sockaddr*)&client_addr, &client_len);
                if (client_socket == -1)
                {
                    std::cout << "Client connection attempt failed!\n";
                    // TODO: close all epoll fds needed
                    close(server_socket);;
                    //close(epollFd);
                    exit(1);
                }
                if (fcntl(client_socket, F_SETFL,
                            fcntl(client_socket, F_GETFL, 0) | O_NONBLOCK) == -1)
                {
                    std::cerr << "ERROR: Couldn't set client_socket "
                        << "to nonblocking\n";
                    close(epollFd);
                    exit(1);
                }
                std::cout << "New client detected!\n";
                std::cout << "Client socket set to nonblocking\n";
                // this sets the notification system
                eEvent.events = EPOLLIN | EPOLLET;
                eEvent.data.fd = client_socket;
                if (epoll_ctl(epollFd, EPOLL_CTL_ADD,
                            client_socket, &eEvent) == -1)
                {
                    //TODO: close all epoll fds needed
                    std::cerr << "ERROR: epoll_ctl failed!\n";
                    close(server_socket);
                    //close(epollFd);
                    exit(1);
                }
                std::cerr << "Client saved!\n";
            }
            else
            {
                received = recv(eEvents[n].data.fd, request, BUFFER_SIZE - 1, 0);
                if (received == 0)
                {
                    close(eEvents[n].data.fd);
                }
                if (received > 0)
                {
                    request[received] = '\0';
                    std::cout << "\nREQUEST\n" << request << "\n\n";
                    respond(eEvents[n].data.fd, request, header, file);
                }
                if (received == -1)
                {
                    std::cerr << "ERROR: received failed!\n";
                    close(eEvents[n].data.fd);
                    close(server_socket);
                    close(epollFd);
                    exit(1);
                }
            }
        }
    }
    close (eEvents[0].data.fd);
    close(epollFd);
    close(server_socket);
    std::cout << "hello!\n";
    return (0);
}

/* TODO:
 *      1- Make it so the port is properly closed at the closing
 *          of the program
 *      2- Start building the configuration file logic with respond
 *          function
 *          */
void respond(int fd, char *request, char *header, char *file)
{
    if (std::strstr(request, "html"))
    {
        file = html_home;
        header = http_header;
    }
    else if (std::strstr(request, ".css"))
    {
        file = css_home;
        header = css_header;
    }
    else if (std::strstr(request, ".js"))
    {
        file = js_home;
        header = js_header;
    }
    if (file && header)
        send_file(fd, file, header);
    close(fd);
}
