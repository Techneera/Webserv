#include "event_loop.hpp"
#include "signal.hpp"

int main(int argc, char **argv)
{
    signalInit();

    return event_loop(argc, argv);
}
