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
static int  idxBlock = 0;
static int  idxSimple = 0;

// what: Look Up table to find if the directive is a valid one
// type: std::map<std::string, enum>
static cfg::d_table directiveSimple;
static cfg::d_table directiveBlock;

// Responsability: Populating the Look Up table as c++98 doesn't allow
//                  Initialization of a map container
void    cfg::configuration_file_init()
{
    directiveSimple["server"] = D_SIMPLE;
    directiveSimple["region"] = D_SIMPLE;
    directiveSimple["host"] = D_SIMPLE;
    directiveSimple["port"] = D_SIMPLE;
    directiveSimple["client_max_body_size"] = D_SIMPLE;
    directiveSimple["root"] = D_SIMPLE;
    directiveSimple["error_pages"] = D_SIMPLE;

    directiveBlock["server"] = D_BLOCK;
    directiveBlock["location"] = D_BLOCK;
    directiveBlock["cgi"] = D_BLOCK;
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
    int     i = utils::peekAndGet(ifs, c);

    while (i != EOF)
    {
        if (i == '\n')
        {
            i = utils::consumeNewLines(ifs, c);
        }
        else if (i == '#')
        {
            i = utils::consumeComments(ifs, c);
        }
        else if (std::isspace(i))
        {
            i = utils::consumeSpaces(ifs, c);
        }
        else
            break ;
    }

    return i;
}

// Responsability: Parsing and populating the simple directive struct
// Algorithm:
// TODO:
//      1- Figure out when to skip comments
static
void    stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
{
    int             i = utils::peekAndGet(ifs, c);
    std::string     arg;
    utils::sVector  args;

    while (i != ';')
    {
        if (i == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if((i = consumeNonToken(ifs, c)) == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        arg = c;
        while (!(i = std::isspace(utils::peekAndGet(ifs,c))))
        {
            if (i == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveSimple));
            if (i != ';')
                break ;
            arg += i;
        }
        args.push_back(arg);
    }

    cfg::d_table::iterator    it;

    // This verification might be more suitable to the server module
    if ((it = directiveBlock.find(args[0])) == directiveBlock.end())
        throw (static_cast<int>(cfg::err_BadDirectiveSimple));

    directivePool[idxBlock].dSimple[idxSimple].name = args[0];
    directivePool[idxBlock].dSimple[idxSimple].args.insert(
            directivePool[idxBlock].dSimple[idxSimple].args.end(),
            args.begin(), args.end());
    idxSimple++;
}

// Responsability: Parsing and populating the simple block struct
// Algorithm:
// TODO:
//      1- Figure out when to skip comments
//      2 - Maybe it makes sense to create a specific data
//         structure to save all the information, since when 
//         the directive name is valid, it wont return the 
//         result until the whole directive is parsed.
//         Maybe it makes sense to build a whole directive block
//         data strucutre and the directive itself is the tree
//      3 - AGAIN, probably it's better to build a throw system and
//         just return the  whole directivePool at the end or NULL
    static
void    stt_parseDirectiveBlock(std::ifstream &ifs, char &c)
{
    bool                        newToken;
    int                         i;
    int                         argCount = 1;
    std::string                 name;
    std::string                 argument;

    if((i = consumeNonToken(ifs, c)) == EOF)
        throw (static_cast<int>(cfg::err_BadDirectiveSimple));

    while (!(i = std::isspace(utils::peekAndGet(ifs, c))))
    {
        if (i == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));
        name += c;
    }

    cfg::d_table::iterator      it;

    if ((it = directiveSimple.find(name)) == directiveSimple.end())
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (name != "location")
        goto:

            if((i = consumeNonToken(ifs, c)) == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveBlock));
    argument = i;

    while (!(i = std::isspace(utils::peekAndGet(ifs, c))))
    {
        if (i == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));
        argument += c;
    }

    while ((i = (utils::peekAndGet(ifs, c))) != '{')
    {
        if (i == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        newToken = true;
        while (!(i = std::isspace(utils::peekAndGet(ifs, c))))
        {
            if (i == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveBlock));
            if (newToken == true)
            {
                argCount++;
                argument += c;
            }
            newToken = false;
        }
        if (argCount > 1)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));
    }

    if (argCount != 1)
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (i == EOF)
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (it->second == cfg::D_SIMPLE)
        stt_parseDirectiveBlock(ifs, c);
    if (it->second == cfg::D_BLOCK)
        stt_parseDirectiveBlock(ifs, c);

    if (c != '}')
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));
}

// TODO:
//      1 - Maybe change to a better name after group discussion
//      2 - Understand when to use the consumers as sometimes
//          the spaces are meaninful, for example, when trying
//          to understand what's the name of the directive
int  cfg::fetch_config(char *filename)
{
    cfg::configuration_file_init();

    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        return (static_cast<int>(err_FileOpening));
    }

    cfg::D_Block    *directive;
    std::string     digits;
    char            c;
    //bool            started;

    directive = stt_parseDirectiveBlock(ifs, c);
    if (directive == NULL)
        throw (cfg::err_BadDirective);

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
    return (8000);
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
        default:
            std::cout << "unknown cause!\n";
            break ;
    }
}

