#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.hpp"
#include "configuration_file.hpp"

// TODO: replace NULL RETURNS WITH ERROR THROW

// what: static allocated memory pool to save the config file directives
// type: 
//      typedef struct d_block
//      {
//          bool            free;
//          d_type          type;
//          std::string     name;
//          std::string     argument;
//          D_Simple        dSimple[MAX_S_DIRECTIVE_SIZE];
//          struct d_block  *next;
//      }   D_Block;
// TODO:
//      Might just not return the nodes on the parseDirective funcitons
//      and just throw the errors if the actual static allocation works
static cfg::D_Block directivePool[MEMORY_POOL_SIZE];

// what: global idex to iterate the memory pool
static int  idxBlock = -1;
static int  idxSimple = 0;

// what: Look Up table to find if the directive is a valid one
// type: std::map<std::string, enum>
static cfg::d_table directiveTable;

// Responsability: Populating the Look Up table as c++98 doesn't allow
//                  Initialization of a map container
// TODO:
//      1 - Maybe make the look up table have more values for multiple
//          keys so theres further validation of each directive
void    cfg::configuration_file_init()
{
    directiveTable["listen"] = D_SIMPLE;
    directiveTable["region"] = D_SIMPLE;
    directiveTable["host"] = D_SIMPLE;
    directiveTable["port"] = D_SIMPLE;
    directiveTable["client_max_body_size"] = D_SIMPLE;
    directiveTable["root"] = D_SIMPLE;
    directiveTable["error_pages"] = D_SIMPLE;

    directiveTable["server"] = D_BLOCK;
    directiveTable["location"] = D_BLOCK;
    directiveTable["cgi"] = D_BLOCK;
}

// 1 - Assigns next char to c
// 2 - Returns next char through c
static
int stt_get(std::ifstream &ifstream, char &c)
{
    if (!ifstream.get(c))
        return (EOF);

    return (c);
}

// Responsability: Consumes Spaces of a ifstream
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

// Responsability: Consumes Comments of a ifstream
static
int stt_consumeComments(std::ifstream &ifs, char &c)
{
    int     i;

    while ((i = stt_get(ifs, c)) != '\n')
        if (i == EOF)
            break ;

    return (i);
}

static
int stt_consumeNewLines(std::ifstream &ifs, char &c)
{
    int i;

    while ((i = stt_get(ifs, c)) == '\n');

    return (i);
}

// Responsability: Consuming any blocks of characters that are
//                  not relevant for the webserver configuration
// Algorithm: 1 - Check which is the current char
//            2 - If current char is '\n', consume any '\n' lines
//            3 - If current char is '#', consume comment line
//            4 - If current char is '<space>', consume spaces
//            5 - Else,  it is the start of a token, so return char
int consumeNonToken(std::ifstream &ifs, char &c)
{
    int i;

    if  (c != '\n' && c != '#' && !std::isspace(c))
        return  (c);

    while (c != EOF)
    {
        if (c == '\n')
        {
            i = stt_consumeNewLines(ifs, c);
        }
        else if (c == '#')
        {
            i = stt_consumeComments(ifs, c);
        }
        else if (std::isspace(c))
        {
            i = stt_consumeSpaces(ifs, c);
        }
        else
            break ;
    }

    return i;
}

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

    static
int stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
{
    std::string     arg;
    utils::sVector  args;

    while (1)
    {
        if (c == '#' || c == '\n' || c == '}' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if (stt_consumeSpaces(ifs, c) == EOF)
            return (EOF);

        if (c == '#' || c == '\n' || c == ';' || c == '}'
                || c == '{')
            return (1);

        if (stt_parseToken(ifs, c, arg) == EOF)
            return (EOF);

        if (c == '}' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if (c == ';')
            return (stt_get(ifs, c));
    }
}

    static
int stt_parseDirectiveBlock(std::ifstream &ifs, char &c)
{
    if (c != '{')
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (!ifs.get(c))
        return (EOF);

    std::string     arg;
    utils::sVector  args;

    while (c != '}')
    {
        if (consumeNonToken(ifs, c) == EOF)
            return (EOF);

        if (c == '}')
            return (stt_get(ifs, c));

        if (c == ';' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (stt_parseToken(ifs, c, arg) == EOF)
            return (EOF);

        cfg::d_table::iterator      it;

        if ((it = directiveTable.find(arg)) == directiveTable.end())
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (it->second == cfg::D_BLOCK)
        {
            if (consumeNonToken(ifs, c) == EOF)
                return (EOF);

            if (arg == "location")
            {
                if (stt_parseToken(ifs, c, arg) == EOF)
                    return (EOF);

                if (consumeNonToken(ifs, c) == EOF)
                    return (EOF);
            }
            stt_parseDirectiveBlock(ifs, c);
        }
        else if(it->second == cfg::D_SIMPLE)
        {
            stt_parseDirectiveSimple(ifs, c);
        }
        else
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));
    }

    stt_get(ifs, c);

    return (1);
}

    static
int stt_parseConfig(std::ifstream &ifs, char &c)
{
    std::string arg;

    while (1)
    {
        if (!ifs.get(c))
            return (EOF);

        if (consumeNonToken(ifs, c) == EOF)
            return (EOF);

        if (c == ';' || c == '}' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirective));

        if (stt_parseToken(ifs, c, arg) == EOF)
            return (EOF);

        cfg::d_table::iterator      it;

        if ((it = directiveTable.find(arg)) == directiveTable.end())
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (it->second == cfg::D_BLOCK)
        {
            if (consumeNonToken(ifs, c) == EOF)
                return (EOF);
            stt_parseDirectiveBlock(ifs, c);
        }
        else
            throw (static_cast<int>(cfg::err_BadConfig));
    }
}

// TODO:
//      1 - Maybe change to a better name after group discussion
//      2 - Understand when to use the consumers as sometimes
//          the spaces are meaninful, for example, when trying
//          to understand what's the name of the directive
cfg::D_Block  *cfg::fetch_config(char *filename)
{
    cfg::configuration_file_init();

    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        throw (static_cast<int>(err_FileOpening));
    }

    std::string     digits;
    char            c;
    //bool          started;

    stt_parseConfig(ifs, c);

    return (directivePool);

    // (RAFA CODE) TODO: 
    //      1- Move to another function that parses ports
    /*if (std::isdigit(static_cast<unsigned char>(c)))
      {
      digits += c;
      started = true;
      }
      else if (started)
      break; */

    // (RAFA CODE) TODO: 
    //      1- Move to another function that parses ports
    /*if (digits.empty())
      {
      std::cerr << "ERROR: No port number found in config"
      << std::endl;
      return (-1);
      }

      errno = 0;
      char    *end = NULL;
      long    port = std::strtol(digits.c_str(), &end, 10);

      if (errno == ERANGE)
      {
      std::cerr << "ERROR: Port number out of range"
      << std::endl;
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

      return (static_cast<int>(port));*/
}

void    cfg::errorPrint(int code)
{
    std::cout << "Error: ";
    switch (code)
    {
        case err_BadDirective:
            std::cout << "Bad directive!\n";
            break ;
        case err_BadDirectiveSimple:
            std::cout << "Bad simple directive structure!\n";
        case err_BadDirectiveBlock:
            std::cout << "Bad block directive structure!\n";
        case err_BadConfig:
            std::cout << "Expecting block directive!\n";
        default:
            std::cout << "unknown cause!\n";
            break ;
    }
}

