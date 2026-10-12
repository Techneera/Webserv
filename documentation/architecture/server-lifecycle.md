# Server Lifecycle

1. Create an endpoint with the socket() system call
```
#include <sys/socket.h>

        int socket(int domain, int type, int protocol);
```
```
        int fd = socket(AF_INET, SOCK_STREAM, 0);
```

2. Claim a port with the bind() system call
```
struct sockaddr
{
    sa_family_t sa_family; /* Address family */
    char        sa_data[]; /* Socket address */
};

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

3. Start waiting for a request with listen() system call
```
int listen(int sockfd, int backlog);
```

4. Accept a client with accept() system call
```
int accept(int sockfd, struct sockaddr *addr, socklen_t addrlen);
```

5. Exchange data with 
    - recv() which copies bytes from the kernel's network buffer into the program. Return 0 when the client closes the connection, -1 on error.
    - send() copies bytes from the program to the kernel, which then transmits them over the network.
```
ssize_t recv(int sockfd, void buf[size], size_t size, int flags, struct sockaddr *src_addr, socklen_t addrlen);

ssize_t send(int sockfd, const void buf[size], size_t size, int flags);
```

6. Close the connections
```
#include <unistd.h>

int close(int fd);
```
