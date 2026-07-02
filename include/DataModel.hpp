#ifndef DATAMODEL_HPP
#define DATAMODEL_HPP

#define MAX_EPOLL_EVENTS 2048
#define MAX_INPUT_PIPES  32
#define MAX_BUFFER_SIZE  1024

typedef struct TYPE_chunk
{
    int     n;
    char    arr[MAX_BUFFER_SIZE];
} chunck_t;

#endif
