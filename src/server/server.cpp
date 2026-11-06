#include "server.hpp"
#include "utils.hpp"
#include <cstdlib>

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

static
int portGet(const cfg::D_Simple &port)
{
    if (port.args.size() != 1)
        throw(static_cast<int>(utl::err_BadPort));

    std::string     digits;

    errno = 0;
    char    *end = NULL;
    long    portNo = strtol(port.args[0].c_str(), &end, 10);

    if (errno == ERANGE)
    {
        return (-1);
    }
    if (end == digits.c_str() || *end != '\0')
    {
        throw (static_cast<int>(utl::err_BadPortRange));
    }
    if (portNo < 1025 || portNo > 65535)
    {
        throw (static_cast<int>(utl::err_BadPortRange));
    }

    return (static_cast<int>(portNo));
}

// TODO:
//      1- Bit Hash table to determine if all the mandatory
//              directives are filled in
srv::Config::Config(const cfg::D_Block &server)
{
    /*if (server.dSimple.size()) < MIN_CFG_DSIMPLE_SIZE
     *      throw (static_cast<int>());*/

    this->region = region;
    this->host = host;
    this->errorPages = errorPages;
    this->location = location;
    this->clientMaxBodySize = clientMaxBodySize;

    for (size_t i = 0; i < server.dSimple.size(); i++)
    {
        cfg::D_Simple   dSimple = server.dSimple[i];
        std::string     dName = dSimple.name;

        if (dSimple.name == "port")
        {
            this->port = portParse(server.dSimple[i]);
        }
        else if (dSimple == region)
        {
            this->region = regionParse(dSimple);
        }
        else if (dSimple == host)
        {
            this->host = hostParse(dSimple);;
        }
        else if (dSimple == error_pages))
        {
            this->errorPages = error_pages;
        }
    }
}
