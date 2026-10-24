#include "event_loop.hpp"
#include "config_tmp.hpp"

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

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int event_loop(int argc, char **argv)
{
  if (argc != 2)
  {
      std::cerr << "ERROR: Usage: " << argv[0]
                << " <config_file>" << std::endl;
      return (1);
  }
  std::cout << "Setting " << argv[1]
            << " as the configuration file\n";

  // TODO:
  //       1 -
  int port = fetch_config(argv[1]);
  if (port < 0)
  {
      std::cerr << "ERROR: Failed reading port from config"
                << std::endl;
      return (-1);
  }
  
  int listener_fd = createListener(port);
  int e_fd = epoll_create(1);
  if (e_fd < 0)
  {
    std::cerr << "ERROR: Failed to create epoll" << std::endl;
    return (1);
  }
  
  struct epoll_event ev;
  ev.events = EPOLLIN; // Only read
  ev.data.fd = listener_fd;

  if (epoll_ctl(e_fd, EPOLL_CTL_ADD, listener_fd, &ev) < 0)
  {
      std::cerr << "ERROR: epoll_ctl failed: " << std::strerror(errno) << std::endl;
      return (-1);
  }

  std::cout << "Listening to port " << port << std::endl;

  struct epoll_event events[MAX_EVENTS];
  while (1)
  {
      int n = epoll_wait(e_fd, events, MAX_EVENTS, -1);
      if (n < 0)
      {
          if (errno == EINTR) // Signal interrupt just retry
              continue;
          std::cerr << "ERROR: epoll_wait failed: " << std::strerror(errno) << std::endl;
          break;
      }

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;

      if (fd == listener_fd)
      {
          while (1)
          {
              int client_fd = accept(listener_fd, NULL, NULL);
              if (client_fd < 0)
              {
                  if (errno != EAGAIN && errno != EWOULDBLOCK)
                      std::cerr << "ERROR: epoll_wait failed: " << std::strerror(errno) << std::endl;
                  break;
              }
              setNonBlocking(client_fd);
              ev.events = EPOLLIN;
              ev.data.fd = client_fd;
              epoll_ctl(e_fd, EPOLL_CTL_ADD, client_fd, &ev);
              std::cout << "[+] client fd=" << client_fd << " accepted" << std::endl;
          }
      }
      else
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
      }
    }
  }
  return (0);
}

