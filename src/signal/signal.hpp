#pragma once

#include <sys/signal.h>

// GLOBALS ---------------------------------------------------
extern volatile sig_atomic_t    interrupt;


// FUNCTIONS -------------------------------------------------
void signalInit();
