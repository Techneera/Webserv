#include "config.hpp"
#include "utils.hpp"
#include <cstdlib>

// Default Constructor
srv::Config::Config()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CONFIG);

    this->port = 0;
    this->ip = "Unknown";
    this->errorPages = "Unknown";
    this->clientMaxBodySize = "Unknown";
}

// Copy Constructor
srv::Config::Config(const Config& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CONFIG);
    this->port = other.port;
    this->ip = other.ip;
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
        this->ip = other.ip;
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
int stt_portGet(const utl::sVector &portArgs)
{
    if (portArgs.size() != 1)
        throw(static_cast<int>(utl::err_BadPort));

    std::string     digits;

    errno = 0;
    char    *end = NULL;
    long    portNo = strtol(portArgs[0].c_str(), &end, 10);

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
srv::Config::Config(const cfg::D_Block &server)
{
    // Simple Directives
    cfg::map_drtSimple              dSimple = server.dSimple;
    cfg::map_drtSimple::iterator    itSimple;
    cfg::map_drtSimple::iterator    dSimpleEnd = dSimple.end();

    if (dSimple.size() != 4)
        throw(static_cast<int>(utl::err_BadDirectiveSimple));

    if ((itSimple = dSimple.find(cfg::e_listen)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingPort));
    this->port = stt_portGet(itSimple->second);

    /*if ((itSimple = dSimple.find(cfg::e_host)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingIp));
    this->ip = stt_ipGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_client_max_body_size)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingClientMaxBodySize));
    this->clientMaxBodySize = stt_clientMaxBodySizeGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_error_pages)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingErrorPages));
    this->errorPages = stt_errorPagesGet(itSimple->second);

    // Block Directives
    cfg::map_drtBlock               dBlock = server.dBlock;
    cfg::map_drtBlock::iterator     itBlock;
    cfg::map_drtBlock::iterator     dBlockEnd = dBlock.end();

    if ((itBlock = dBlock.find(cfg::e_location)) == dBlockEnd)
        throw(static_cast<int>(utl::err_MissingLocation));
    this->location = location = stt_locationGet(itBlock->second);*/
}
