#include "event_loop.hpp"
#include "config_tmp.hpp"
#include "signal.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <stdio.h>

/* TODO:
 *      1- transform these hardcoded headers and paths to 
 *          harcoded populated structs according to Julio's
 *          architecture suggestion */
char        html_home[] = "assets/data/www/homepage/homepage.html";
char        css_home[] = "assets/data/www/homepage/homepage.css";
char        js_home[] = "assets/data/www/homepage/homepage.js";
char        http_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\
                             Connection: close\r\n\r\n";
char        css_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\
                            Connection: close\r\n\r\n";
char        js_header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/js\r\n\
                            Connection: close\r\n\r\n";

/* TODO:
 *      1- Maybe at the end the signal would be at the main.cpp file
    so it is only mentioned there and the signal functions too */
volatile sig_atomic_t   interrupt = 0;

void respond(int fd, char *request, char *header, char *file);

/* TODO:
 *      1- Check if all the fd's are being properly closed in case
 *          of error or SIGINT */
static int createListener(int port)
{
  int fd;
  int opt;
  struct sockaddr_in  addr;

  // Added non block option in the socket definition
  fd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0);
  if (fd < 0)
  {
    std::cerr << "ERROR: Could not create socket "
              << std::strerror(errno) << std::endl;
    exit(1);
  }
  std::cout << "Server socket created with success\n";

  opt = 1; // SO_REUSEADDR restart the server without having to restart port
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    std::cerr << "ERROR: Could not set option for socket "
              << std::strerror(errno) << std::endl;
    exit(1);
  }
  std::cout << "Server socket options set to: SOL_SOCKET, SO_REUSEADDR\n";

  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 all interfaces
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
  {
    std::cerr << "ERROR: Failed to set server socket to "
              << "IPv4 protocol with port " << port
              << std::strerror(errno) << std::endl;
    exit(1);
  }
  std::cout << "Server socket set to IPv4 protocol with port "
            << port << "\n";

  if (listen(fd, SOMAXCONN) < 0)
  {
    std::cerr << "ERROR: Failed to set socket to passive mode "
              << std::strerror(errno) << std::endl;
    exit(1);
  }
  std::cout << "Server socket set to passive\n";

  //fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}

// Changed function to return int for error check purposes
static int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    else
        return (-1);
}

int event_loop(int argc, char **argv)
{
  /*if (argc != 2)
  {
      std::cerr << "ERROR: Usage: " << argv[0]
                << " <config_file>" << std::endl;
      return (1);
  }
  std::cout << "Setting " << argv[1]
            << " as the configuration file\n";*/

  int port = 8000;
  // TODO:
  //        1 - Create config file parser function
  //        2 - Maybe give it struct address to populate with 
  //            necessary instructions/info suchs as port,
  //            or file dir
  /*int port = fetch_config(argv[1]);
  if (port < 0)
  {
      std::cerr << "ERROR: Failed reading port from config"
                << std::endl;
      return (-1);
  }*/
  
  int listener_fd = createListener(port);
  int e_fd = epoll_create1(0);
  if (e_fd < 0)
  {
    std::cerr << "ERROR: Failed to create epoll" << std::endl;
    return (1);
  }
  std::cout << "Created epoll\n";
  
  struct epoll_event ev;
  ev.events = EPOLLIN; // Only read
  ev.data.fd = listener_fd;

  if (epoll_ctl(e_fd, EPOLL_CTL_ADD, listener_fd, &ev) < 0)
  {
      std::cerr << "ERROR: epoll_ctl failed: "
                << std::strerror(errno) << std::endl;
      return (-1);
  }
  std::cout << "epoll_ctl call success\n";

  struct epoll_event events[MAX_EVENTS];
  while (!interrupt)
  {
      int n = epoll_wait(e_fd, events, MAX_EVENTS, -1);
      if (n < 0)
      {
          if (errno == EINTR) // Signal interrupt just retry
              continue;
          std::cerr << "ERROR: epoll_wait failed: "
                    << std::strerror(errno) << std::endl;
          break;
      }
      std::cout << "Connection found\n";

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;

      if (fd == listener_fd)
      {
          // (Alex) TODO:
          //        1- Understand the reason of this loop as I didn't
          //            do it
          /*while (1)
          {*/
              int client_fd = accept(listener_fd, NULL, NULL);
              if (client_fd < 0)
              {
                  if (errno != EAGAIN && errno != EWOULDBLOCK)
                      std::cerr << "ERROR: epoll_wait failed: "
                                << std::strerror(errno) << std::endl;
                  break;
              }
              if (setNonBlocking(client_fd) == -1)
              {
                    std::cerr << "ERROR: Couldn't set client_socket "
                        << "to nonblocking\n";
                    close(client_fd);
                    return (-1);
              }
              ev.events = EPOLLIN | EPOLLET; // Added EPOLLET
              ev.data.fd = client_fd;
              if (epoll_ctl(e_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1)
              {
                    //TODO: close all epoll fds needed
                    std::cerr << "ERROR: epoll_ctl failed!\n";
                    close(client_fd);
                    close(listener_fd);
                    //close(e_fd);
                    exit(1);
              }
              std::cout << "[+] client fd=" << client_fd << " accepted"
                        << std::endl;
          //}
      }
      else
      {
          int   received;
          char  request[MAX_BUF_SIZE];
          char  *file = NULL;
          char  *header = NULL;

          received = recv(fd, request, sizeof(request) - 1, 0);
          if (received == 0)
          {
              close(fd);
          }
          if (received > 0)
          {
              request[received] = '\0';
              std::cout << "\nREQUEST\n" << request << "\n\n";
              respond(fd, request, header, file);
          }
          if (received == -1)
          {
              std::cerr << "ERROR: received failed!\n";
              close(fd);
              close(listener_fd);
              close(e_fd);
              exit(1);
          }
      }

      /*else
      {
        char buf[MAX_BUF_SIZE];
        ssize_t response = recv(fd, buf, sizeof(buf), 0);

        if (response <= 0)
        {
          epoll_ctl(e_fd, EPOLL_CTL_DEL, fd, NULL);
          close(fd);
          std::cout << "[-] client fd=" << fd << " disconnected" << std::endl;
        }
        else
        {
          send(fd, buf, response, MSG_NOSIGNAL);
        }
      }*/
    }
  }
  return (0);
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

    char        buffer[MAX_BUF_SIZE];
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

    while ((bytes_read = fread(buffer, 1, MAX_BUF_SIZE, file)) > 0)
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
