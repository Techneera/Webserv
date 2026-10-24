#include <sys/signal.h>
#include <iostream>

extern volatile sig_atomic_t   interrupt;

static void signalHandler(int signalCode)
{
    std::cerr << "Server Interrupted by SIGINT!\n";
    interrupt = signalCode;
}

void    signalInit()
{
    struct sigaction    newAction;

    newAction.sa_handler = signalHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;
    sigaction(SIGINT, &newAction, NULL);
}
