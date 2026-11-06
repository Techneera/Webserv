#include "server.hpp"

// Default Constructor
srv::Server::Server()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(SERVER);
}

// Copy Constructor
srv::Server::Server(const Server& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(SERVER);
    *this = other;
}

// Copy Assignment operator
srv::Server &srv::Server::operator=(const Server& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(SERVER);
    return *this;
}

// Destructor
srv::Server::~Server()
{
    std::cout << DESTRUCTOR_MSG(SERVER);
}

srv::Server::Server(const cfg::D_Block dBlock)
{
    this->config = Config(dBlock);
}

