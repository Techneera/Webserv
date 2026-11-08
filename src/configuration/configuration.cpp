#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>


#include "utils.hpp"
#include "configuration.hpp"

// @brief:  Look Up table to find if the directive is a valid one
static cfg::map_drtTable directiveTable;

// @brief:  Populates the Look Up table as c++98 doesn't allow
//                  Initialization of a map container
//
// @PARAM:  none
//
// @return: void
static
void    configuration_file_init()
{
    directiveTable["listen"] = cfg::e_listen;
    directiveTable["host"] = cfg::e_host;
    directiveTable["client_max_body_size"] = cfg::e_client_max_body_size;
    directiveTable["error_pages"] = cfg::e_error_pages;
    directiveTable["root"] = cfg::e_root;
    directiveTable[".rs"] = cfg::e_rust;

    directiveTable["server"] = cfg::e_server;
    directiveTable["location"] = cfg::e_location;
    directiveTable["cgi"] = cfg::e_cgi;
}

// @brief:  Provides a small ease of use modification of
//              the standard ifstream method get()
//
// @PARAM:  ifstream - config file stream; c - holds next char
//
// @return: int (to be able to return both EOF or any char)
static
int stt_get(std::ifstream &ifstream, char &c)
{
    if (!ifstream.get(c))
        return (EOF);

    return (c);
}

// @brief:  Consumes Spaces of a ifstream
//
// @PARAM:  ifstream - config file stream; c - holds next char
//
// @return: int (EOF or next char)
static
int stt_consumeSpaces(std::ifstream &ifs, char &c)
{
    int i;

    while ((i = stt_get(ifs, c)) != EOF)
    {
        if (!std::isspace(c))
            break ;
    }

    return (i);
}

// @brief:  Consumes Comments of a ifstream
//
// @PARAM:  ifstream - config file stream; c - holds next char
//
// @return: int (EOF or next char)
static
int stt_consumeComments(std::ifstream &ifs, char &c)
{
    int     i;

    while ((i = stt_get(ifs, c)) != '\n')
        if (i == EOF)
            break ;

    return (i);
}

// @brief:  Consumes new line chars of a ifstream
//
// @PARAM:  ifstream - config file stream; c - holds next char
//
// @return: int (EOF or next char)
static
int stt_consumeNewLines(std::ifstream &ifs, char &c)
{
    int i;

    while ((i = stt_get(ifs, c)) == '\n');

    return (i);
}

// @brief:      Consumes any blocks of characters that are
//                  not relevant for the webserver configuration
//                  such as comments, new lines and spaces
//
// @PARAM:      ifstream - config file stream; c - holds next char
//
// @return:     int (EOF or next char)
//
// @details:    1 - Check which is the current char
//              2 - If current char is '\n', consume any '\n' lines
//              3 - If current char is '#', consume comment line
//              4 - If current char is '<space>', consume spaces
//              5 - Else,  it is the start of a token, so return char
static
int stt_consumeNonToken(std::ifstream &ifs, char &c)
{
    if  (c != '\n' && c != '#' && !std::isspace(c))
        return  (c);

    while (1)
    {
        if (c == '\n')
        {
            if (stt_consumeNewLines(ifs, c) == EOF)
                return (EOF);
        }
        else if (c == '#')
        {
            if (stt_consumeComments(ifs, c) == EOF)
                return  (EOF);
        }
        else if (std::isspace(c))
        {
            if (stt_consumeSpaces(ifs, c) == EOF)
                return  (EOF);
        }
        else
            break ;
    }

    return (1);
}

// @brief:  Saves next token in <token> param. A token is any
//             block of the config file ifstream that is
//             separated by spaces, is not a comment, is not
//             a new line, EOF or any special character that
//             determines the end or the beggining of a simple
//             or block directive
//
// @PARAM:  ifstream - config file stream; c - holds next char;
//             token - token to be saved
//
// @return: int (EOF or next char)
static
int stt_parseToken(std::ifstream &ifs, char &c, std::string &token)
{
    token = "";

    while (c != '#' && c != '\n' && c != ';' && c != '}'
            && c != '{' && !std::isspace(c))
    {
        token += c;
        if (stt_get(ifs, c) == EOF)
            return EOF;
    }

    return (1);
}

// @brief:  parses ifstream until the char ';' which determines
//              the end of a simple directive section
//
// @PARAM:  ifstream - config file stream; c - holds next char;
//              token - token to be saved
//
// @return: vector<std::string> that holds the arguments that
//              succeed the directive name
//
// @details:    1 - While the char ';' is not found
//              2 -     Parse next token
//              3 -     Add token to a tokens vector
//              4 - Return the tokens vector
static
utl::sVector stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
{
    std::string     arg;
    utl::sVector  args(0);

    while (1)
    {
        if (c == '#' || c == '\n' || c == '}' || c == '{')
            throw (static_cast<int>(utl::err_BadDirectiveSimple));

        if (stt_consumeSpaces(ifs, c) == EOF)
            throw (static_cast<int>(utl::err_BadDirectiveSimple));

        if (c == '#' || c == '\n' || c == ';' || c == '}'
                || c == '{')
            throw (static_cast<int>(utl::err_BadDirectiveSimple));

        if (stt_parseToken(ifs, c, arg) == EOF)
            throw (static_cast<int>(utl::err_BadDirectiveSimple));

        args.push_back(arg);

        if (c == '}' || c == '{')
            throw (static_cast<int>(utl::err_BadDirectiveSimple));

        if (c == ';')
        {
            stt_get(ifs, c);
            return (args);
        }
    }
}

// @brief:      parses ifstream until the char '}' which determines
//                  the end of a Block Directive section. It provides
//                  the parsing of nested Block Directives
//
// @PARAM:      ifstream - config file stream; c - holds next char;
//                  token - token to be saved
//
// @return:     cfg::D_Block that holds the parsed Directive Block.
//
// @details:    1 - While the char '}' is not found
//              2 -     Parse next token
//              3 -     If token is Block Directive
//              4 -         Set token as Block Directive name
//              5 -         Parse Block Directive and save it in the current block
//              6 -         directive structure
//              7 -     Else If token is Simple Directive
//              8 -         Set token as Simple Directive name
//              9 -         Parse Simple Directive and save it in the current block
//              a -         directive structure
//              b -     Else
//              c -         Throw an error
//              d - Return the Block Directive
static
cfg::D_Block    stt_parseDirectiveBlock(std::string _name, std::ifstream &ifs,
                                        char &c)
{
    cfg::D_Block directiveBlock;

    if (c != '{')
        throw (static_cast<int>(utl::err_BadDirectiveBlock));

    if (!ifs.get(c))
        throw (static_cast<int>(utl::err_BadDirectiveBlock));


    directiveBlock.name = _name;

    std::string     name;

    while (c != '}')
    {
        if (stt_consumeNonToken(ifs, c) == EOF)
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        if (c == '}')
        {
            stt_get(ifs, c);
            break ;
        }

        if (c == ';' || c == '{')
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        if (stt_parseToken(ifs, c, name) == EOF)
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        cfg::map_drtTable::iterator      it;

        if ((it = directiveTable.find(name)) == directiveTable.end())
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        if (it->second & cfg::e_dir_block)
        {
            if (stt_consumeNonToken(ifs, c) == EOF)
                throw (static_cast<int>(utl::err_BadDirectiveBlock));

            if (name == "location")
            {
                if (stt_parseToken(ifs, c, name) == EOF)
                    throw (static_cast<int>(utl::err_BadDirectiveBlock));

                if (stt_consumeNonToken(ifs, c) == EOF)
                    throw (static_cast<int>(utl::err_BadDirectiveBlock));
            }

            directiveBlock.dBlock.insert(std::make_pair(
                        it->second, stt_parseDirectiveBlock(name, ifs, c)));

        }
        else if(it->second & cfg::e_dir_simple)
        {
            directiveBlock.dSimple.insert(std::make_pair(
                        it->second, stt_parseDirectiveSimple(ifs, c)));
        }
        else
            throw (static_cast<int>(utl::err_BadDirectiveBlock));
    }

    stt_get(ifs, c);

    return  (directiveBlock);
}

// @brief:      parses multiple server Block Directives and saves it
//                  int a vector
//
// @PARAM:      ifstream - config file stream; c - holds next char;
//
// @return:     vector<cfg::D_Block>, an array of multiple D_Blocks that
//                  holds the server blocks which will provide the server
//                  class the necessary information for its instantiations
//
// @details:    1 - While theres characters in ifstream
//              2 -     get next token
//              3 -     If next token is "server"
//              4 -         parse server as a Block Directive
//              5 -     Else
//              6 -         throw an error
//              7 - Return Block Directives vector
static
std::vector<cfg::D_Block> stt_parseConfig(std::ifstream &ifs, char &c)
{
    std::string                 arg;
    std::vector<cfg::D_Block>   config;

    while (1)
    {
        if (!ifs.get(c))
            return (config);

        if (stt_consumeNonToken(ifs, c) == EOF)
            return (config);

        if (c == ';' || c == '}' || c == '{')
            throw (static_cast<int>(utl::err_BadConfig));

        if (stt_parseToken(ifs, c, arg) == EOF)
            throw (static_cast<int>(utl::err_BadConfig));

        if (arg != "server")
            throw (static_cast<int>(utl::err_BadConfig));

        if (stt_consumeNonToken(ifs, c) == EOF)
            return (config);

        config.push_back(stt_parseDirectiveBlock("server", ifs, c));
    }
}

// @brief:  opens the configuration file and calls it's parser function
//
// @PARAM:  filename - webserver configuration file path
//
// @return: vector<cfg::D_Block>, an array of multiple D_Blocks that
//              holds the server blocks which will provide the server
//              class the necessary information for its instantiations
std::vector<cfg::D_Block>  cfg::fetch_config(char *filename)
{
    configuration_file_init();

    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        throw (static_cast<int>(utl::err_FileOpening));
    }

    std::string     digits;
    char            c;

    return (stt_parseConfig(ifs, c));
}

// Default Constructor
cfg::Config::Config()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CONFIG);

    this->port = 0;
    this->ip = "Unknown";
    this->clientMaxBodySize = 0;
}

// Copy Constructor
cfg::Config::Config(const Config& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CONFIG);
    this->port = other.port;
    this->ip = other.ip;
    this->errorPages = other.errorPages;
    this->location = other.location;
    this->clientMaxBodySize = other.clientMaxBodySize;
}

// Copy Assignment operator
cfg::Config& cfg::Config::operator=(const Config& other)
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
cfg::Config::~Config()
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

static
std::string stt_ipGet(const utl::sVector &ipArgs)
{
    if (ipArgs.size() != 1)
        throw(static_cast<int>(utl::err_BadIp));

    return (ipArgs[0]);
}

// TODO:
//      1- Parse numbers then one letter
//      2- Parse only the letters k, m, g and only one letter
//      3- Multiple number by letter multiplier representation
//      4- Make an enum
static
long    stt_clientMaxBodySizeGet(const utl::sVector &clientMaxBodySizeArgs)
{
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
utl::lsMap stt_errorPagesGet(const utl::sVector &errorPagesArgs)
{
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

    closedir(errorPagesDir);

    return (errorPagesResponses);
}

// TODO:
cfg::Config::Config(const cfg::D_Block &server)
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

    if ((itSimple = dSimple.find(cfg::e_host)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingIp));
    this->ip = stt_ipGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_client_max_body_size)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingClientMaxBodySize));
    this->clientMaxBodySize = stt_clientMaxBodySizeGet(itSimple->second);

    if ((itSimple = dSimple.find(cfg::e_error_pages)) == dSimpleEnd)
        throw(static_cast<int>(utl::err_MissingErrorPages));
    this->errorPages = stt_errorPagesGet(itSimple->second);

    // Block Directives
    /*cfg::map_drtBlock               dBlock = server.dBlock;
    cfg::map_drtBlock::iterator     itBlock;
    cfg::map_drtBlock::iterator     dBlockEnd = dBlock.end();

    if ((itBlock = dBlock.find(cfg::e_location)) == dBlockEnd)
        throw(static_cast<int>(utl::err_MissingLocation));
    this->location = location = stt_locationGet(itBlock->second);*/
}

