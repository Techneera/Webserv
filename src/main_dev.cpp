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
#include <fstream>
#include <string>

#define MAX_EVENTS  64
#define MAX_BUF_SIZE 4096

static int createListener(int port)
{
  int fd;
  int opt;
  struct sockaddr_in  addr;

  fd = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, 0); // Added non block option in the socket definition
  if (fd < 0)
  {
    std::cerr << "Could not create socket " << std::strerror(errno) << std::endl;
    exit(1);
  }

  opt = 1; // SO_REUSEADDR restart the server without having to restart port
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    std::cerr << "Could not set option for socket " << std::strerror(errno) << std::endl;
    exit(1);
  }

  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 all interfaces
  addr.sin_port = htons(port);

  if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
  {
    std::cerr << "Failed bind " << std::strerror(errno) << std::endl;
    exit(1);
  }

  if (listen(fd, SOMAXCONN) < 0)
  {
    std::cerr << "Failed listen " << std::strerror(errno) << std::endl;
    exit(1);
  }
  
  //fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}

static int  fetch_config(char *filename)
{
    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "Cannot open config file: " << filename << std::endl;
        return (-1);
    }

    std::string     digits;
    char            c;
    bool            started;

    while (ifs.get(c))
    {
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            digits += c;
            started = true;
        }
        else if (started)
            break;
    }

    if (digits.empty())
    {
        std::cerr << "No port number foung in config" << std::endl;
        return (-1);
    }

    errno = 0;
    char    *end = NULL;
    long    port = std::strtol(digits.c_str(), &end, 10);

    if (errno == ERANGE)
    {
        std::cerr << "Port number out of range" << std::endl;
        return (-1);
    }
    if (end == digits.c_str() || *end != '\0')
    {
        std::cerr << "Invalid port number" << std::endl;
        return (-1);
    }
    if (port < 1025 || port > 65535)
    {
        std::cerr << "Port number must be between 1025 and 65535" << std::endl;
        return (-1);
    }

    return (static_cast<int>(port));
}

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
      std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
      return (1);
  }

  int port = fetch_config(argv[1]);
  if (port < 0)
  {
      std::cerr << "Error reading port from config" << std::endl;
      return (-1);
  }
  
  int listener_fd = createListener(port);
  int e_fd = epoll_create(1);
  if (e_fd < 0)
  {
    std::cerr << "Failed to create epoll" << std::endl;
    return (1);
  }
  
  struct epoll_event ev;
  ev.events = EPOLLIN; // Only read
  ev.data.fd = listener_fd;

  if (epoll_ctl(e_fd, EPOLL_CTL_ADD, listener_fd, &ev) < 0)
  {
      std::cerr << "epoll_ctl failed: " << std::strerror(errno) << std::endl;
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
          std::cerr << "epoll_wait failed: " << std::strerror(errno) << std::endl;
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
                      std::cerr << "epoll_wait failed: " << std::strerror(errno) << std::endl;
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
}
