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

#define MAX_EVENTS  64
#define MAX_BUF_SIZE 4096

static int createListener(int port)
{
  int fd;
  int opt;
  struct sockaddr_in  addr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
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
  
  fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    std::cerr << "Wrong number of arguments.\r" << "Usage: ./webserv <network> <port>" << std::endl;
    return (1);
  }
  
  int portno = std::atoi(argv[2]); // zero defaults to port 8080
  if (portno == 0)
  {
    portno = 8080;
  }

  int listener_fd = createListener(portno);
  int e_fd = epoll_create(1);
  if (e_fd < 0)
  {
    std::cerr << "Failed to create epoll" << std::endl;
    return (1);
  }
  
  struct epoll_event ev;
  ev.events = EPOLLIN; // Only read
  ev.data.fd = listener_fd;

  epoll_ctl(e_fd, EPOLL_CTL_ADD, listener_fd, &ev);
  std::cout << "Listening to port " << portno << std::endl;

  struct epoll_event events[MAX_EVENTS];
  while (1)
  {
    int n = epoll_wait(e_fd, events, MAX_EVENTS, -1);

    if (n < 0)
      continue;

    for (int i = 0; i < n; i++)
    {
      int fd = events[i].data.fd;

      if (fd == listener_fd)
      {
        int client_fd = accept(listener_fd, NULL, NULL);
        if (client_fd < 0)
          continue;
        fcntl(client_fd, F_SETFL, O_NONBLOCK);

        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        epoll_ctl(e_fd, EPOLL_CTL_ADD, client_fd, &ev);
        std::cout << "[+] client fd=" << client_fd << " accepted" << std::endl;
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
