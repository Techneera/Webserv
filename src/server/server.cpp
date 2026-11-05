#include "server.hpp"

// Default Constructor
srv::Config::Config()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CONFIG);

    this->port = 0;
    this->region = "Unknown";
    this->host = "Unknown";
    this->errorPages = "Unknown";
    this->clientMaxBodySize = "Unknown";
}

// Copy Constructor
srv::Config::Config(const Config& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CONFIG);
    this->port = other.port;
    this->region = other.region;
    this->host = other.host;
    this->errorPages = other.errorPages;
    this->location = other.location;
    this->clientMaxBodySize = other.clientMaxBodySize;
}

// Copy Assignment operator
srv::Config& srv::Config::operator=(const Config& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(CONFIG);
    if (this != &other)
    {
        this->port = other.port;
        this->region = other.region;
        this->host = other.host;
        this->errorPages = other.errorPages;
        this->location = other.location;
        this->clientMaxBodySize = other.clientMaxBodySize;
    }
    return (*this);
}

// Destructor
srv::Config::~Config()
{
    std::cout << DESTRUCTOR_MSG(CONFIG);
}

srv::Config::Config(cfg::D_Block *server)
{
    /*this->port = port;
    this->region = region;
    this->host = host;
    this->errorPages = errorPages;
    this->location = location;
    this->clientMaxBodySize = clientMaxBodySize;*/
}

// TO TRY TO USE THIS CODE RAFA IMPLEMENTED in the correct place
int srv::Config::portGet(cfg::D_Simple port)
{
    if (port.args.size() != 1)

    std::string     digits;
    char            c;

    errno = 0;
    char    *end = NULL;
    long    port = std::strtol(digits.c_str(), &end, 10);

    if (errno == ERANGE)
    {
        std::cerr << "ERROR: Port number out of range" << std::endl;
        return (-1);
    }
    if (end == digits.c_str() || *end != '\0')
    {
        std::cerr << "ERROR: Invalid port number" << std::endl;
        return (-1);
    }
    if (port < 1025 || port > 65535)
    {
        std::cerr << "ERROR: Port number must be "
            << "between 1025 and 65535" << std::endl;
        return (-1);
    }

    return (static_cast<int>(port));
}


