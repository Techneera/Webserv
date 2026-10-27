#include "server.hpp"

// Default Constructor
Server::Server()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CLASS);

    this->port = 0;
    this->region = "Unknown";
    this->host = "Unknown";
    this->errorPages = "";
}

// Copy Constructor
Server::Server(const Server& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CLASS);
    this->port = other.port;
    this->region = other.region;
    this->host = other.host;
    this->errorPages = other.errorPages;
    this->location = other.location;
}

// Copy Assignment operator
Server& Server::operator=(const Server& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(CLASS);
    if (this != &other)
    {
        this->port = other.port;
        this->region = other.region;
        this->host = other.host;
        this->errorPages = other.errorPages;
        this->location = other.location;
    }
    return (*this);
}

// Destructor
Server::~Server()
{
    std::cout << DESTRUCTOR_MSG(CLASS);
}

Server::Server(size_t port, std::string region, std::string host,
        std::string errorPages, sMap location)
{
    this->port = port;
    this->region = region;
    this->host = host;
    this->errorPages = errorPages;
    this->location = location;
}
