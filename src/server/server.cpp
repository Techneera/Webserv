#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>

#include "server.hpp"

// Default Constructor
Server::Server()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(SERVER);
}

// Copy Constructor
Server::Server(const Server& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(SERVER);

    this->config = other.config;
}

// Copy Assignment operator
Server &Server::operator=(const Server& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(SERVER);

    if (this != &other)
    {
        this->config = other.config;
    }

    return *this;
}

// Destructor
Server::~Server()
{
    std::cout << DESTRUCTOR_MSG(SERVER);
}

// TODO:
Server::Server(const cfg::s_config &serverConfig)
{
}
