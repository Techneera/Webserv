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

const size_t Server::_errorCodesNo = 40;

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

static
int stt_portGet(const std::vector<utl::sVector> &portVector)
{
    if (portVector.size() != 1)
        throw(static_cast<int>(utl::err_BadPort));

    utl::sVector    portArgs = portVector[0];

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
//      1- Verify ipVector validity
//      2- Maybe use already the ip converter function already
static
std::string stt_ipGet(const std::vector<utl::sVector> &ipVector)
{
    if (ipVector.size() != 1)
        throw(static_cast<int>(utl::err_BadIp));

    if (ipVector[0].size() != 1)
        throw(static_cast<int>(utl::err_BadIp));

    return (ipVector[0][0]);
}

// TODO:
//      1- Parse numbers then one letter
//      2- Parse only the letters k, m, g and only one letter
//      3- Multiple number by letter multiplier representation
//      4- Make an enum
static
long    stt_clientMaxBodySizeGet(
            const std::vector<utl::sVector> &clientMaxBodySizeVector)
{
    if (clientMaxBodySizeVector.size() != 1)
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    utl::sVector    clientMaxBodySizeArgs = clientMaxBodySizeVector[0];

    if (clientMaxBodySizeArgs.size() != 1)
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    errno = 0;
    char        *ptr = NULL;
    const char  *clientMaxBodySizeStr = clientMaxBodySizeArgs[0].c_str();
    size_t      clientMaxBodySizeStrLen = strlen(clientMaxBodySizeStr);
    size_t      size = std::strtol(clientMaxBodySizeStr, &ptr, 10);

    if (errno == ERANGE)
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    if  (*clientMaxBodySizeStr == '-')
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    if (ptr == clientMaxBodySizeStr)
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    if (strlen(ptr) > 1)
        throw(static_cast<int>(utl::err_BadClientMaxBodySize));

    switch (std::toupper(*(ptr++)))
    {
        case 'K':
            if (clientMaxBodySizeStrLen > 14)
                throw(static_cast<int>(utl::err_BadClientMaxBodySize));
            size *= 1024;
            break ;
        case 'M':
            if (clientMaxBodySizeStrLen > 10)
                throw(static_cast<int>(utl::err_BadClientMaxBodySize));
            size *= 1024 * 1024;
            break ;
        case 'G':
            if (clientMaxBodySizeStrLen > 6)
                throw(static_cast<int>(utl::err_BadClientMaxBodySize));
            size *= 1024UL * 1024 * 1024;
            break ;
        case '\0':
            if (clientMaxBodySizeStrLen > 18)
                throw(static_cast<int>(utl::err_BadClientMaxBodySize));
            break ;
        default:
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));
    }

    return (size);
}

// TODO:
//      1- Iterate error pages files in the directory
//      2- Check if all mandatory exist(by name)
//      3- Check if all are html
//      4- Check if the html is runnable(???????)
//      5- Save the page on map_response
static
utl::lsMap stt_errorPagesGet(const std::vector<utl::sVector> &errorPagesVector)
{
    if (errorPagesVector.size() != 1)
        throw(static_cast<int>(utl::err_BadErrorPages));

    utl::sVector    errorPagesArgs  = errorPagesVector[0];

    if (errorPagesArgs.size() != 1)
        throw(static_cast<int>(utl::err_BadErrorPages));

    DIR *errorPagesDir = opendir(errorPagesArgs[0].c_str());

    if (!errorPagesDir)
        throw(static_cast<int>(utl::err_BadErrorPagesDir));

    struct dirent   *entry;
    size_t          errorPagesCount = 0;
    utl::lsMap      errorPagesResponses;

    while ((entry = readdir(errorPagesDir)) != NULL)
    {
        std::string file = entry->d_name;

        if (file == "." || file == "..")
            continue ;

        if (!std::isdigit(file[0]))
            continue ;

        char    *ptr = NULL;
        long    errorCode = std::strtol(file.c_str(), &ptr, 10);

        if (ptr == file)
            continue ;

        if (std::strcmp(ptr, ".html") != 0)
            continue ;

        if ((errorCode >= 400 && errorCode <= 418)
            || (errorCode >= 421 && errorCode <= 426)
            || (errorCode == 428) || (errorCode == 429)
            || (errorCode == 431) || (errorCode == 451)
            || (errorCode >= 500 && errorCode <= 508)
            || (errorCode == 510) || (errorCode == 511))
        {
            //TODO: BUILD THE RESPONSE HERE
            //
            //errorPagesResponses.insert(std::make_pair(
            //          errorCode, responseBuild()));
            errorPagesCount++;
        }
    }

    if (errorPagesCount != Server::_errorCodesNo)
        throw (static_cast<int>(utl::err_BadErrorPagesAmount));

    closedir(errorPagesDir);

    return (errorPagesResponses);
}

// TODO:
Server::Server(const cfg::D_Block &serverDirectiveBlock)
{
    // Simple Directives
    cfg::map_drtSimple              dSimple = serverDirectiveBlock.dSimple;
    cfg::map_drtSimple::iterator    itSimple;
    cfg::map_drtSimple::iterator    dSimpleEnd = dSimple.end();

    if (dSimple.size() != 4)
        throw(static_cast<int>(utl::err_BadDirectiveSimple));

    if ((itSimple = dSimple.find(cfg::e_listen)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingPort));
    config.port = stt_portGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_host)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingIp));
    config.ip = stt_ipGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_client_max_body_size)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingClientMaxBodySize));
    config.clientMaxBodySize = stt_clientMaxBodySizeGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_error_pages)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingErrorPages));
    config.errorPages = stt_errorPagesGet(itSimple->second);

    // Block Directives
    /*cfg::map_drtBlock               dBlock = server.dBlock;
    cfg::map_drtBlock::iterator     itBlock;
    cfg::map_drtBlock::iterator     dBlockEnd = dBlock.end();

    if ((itBlock = dBlock.find(cfg::e_location)) == dBlockEnd)
        throw(static_cast<int>(utl::err_MissingLocation));
    config.location = location = stt_locationGet(itBlock->second);*/
}


