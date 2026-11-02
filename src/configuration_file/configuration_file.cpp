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

// Responsability: Consuming any blocks of characters that are
//                  not relevant for the webserver configuration
// Algorithm: 1 - Check which is the current char
//            2 - If current char is '\n', consume any '\n' lines
//            3 - If current char is '#', consume comment line
//            4 - If current char is '<space>', consume spaces
//            5 - Else,  it is the start of a token, so return char
int consumeNonToken(std::ifstream &ifs, char &c)
{
    int i = ifs.peek();

    if  (i != '\n' && i != '#' && !std::isspace(c))
        return (utils::get(ifs, c));

    while (i != EOF)
    {
        if (c == '\n')
        {
            i = utils::consumeNewLines(ifs, c);
        }
        else if (c == '#')
        {
            i = utils::consumeComments(ifs, c);
        }
        else if (std::isspace(c))
        {
            i = utils::consumeSpaces(ifs, c);
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
        if (utils::get(ifs, c) == EOF)
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
            return (-1);

        if (utils::consumeSpaces(ifs, c) == EOF)
            return (EOF);

        if (c == '#' || c == '\n' || c == ';' || c == '}'
                || c == '{')
            return (1);

        if (stt_parseToken(ifs, c, arg) == EOF)
            return (EOF);

        if (c == '}' || c == '{')
            return (-1);

        if (c == ';')
            return (utils::get(ifs, c));
    }
}

    static
int stt_parseDirectiveBlock(std::ifstream &ifs, char &c)
{
    if (c != '{')
        return (-1);

    if (!ifs.get(c))
        return (EOF);

    std::string     arg;
    utils::sVector  args;

    while (c != '}')
    {
        if (consumeNonToken(ifs, c) == EOF)
            return (EOF);

        if (c == '}')
            return (utils::get(ifs, c));

        if (c == ';' || c == '{')
            return (-1);

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
            return (-1);
    }

    utils::get(ifs, c);

    return (1);
}

    static
int stt_parseConfig(std::ifstream &ifs, char &c)
{
    std::string arg;

    while (1)
    {
        if (consumeNonToken(ifs, c) == EOF)
            return (EOF);

        if (c == ';' || c == '}' || c == '{')
            return (-1);

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
void    stt_parseDirective(std::ifstream &ifs, char &c)
{
    int             i = consumeNonToken(ifs, c);
    std::string     arg;
    utils::sVector  args;


    if (c == '}')
        return ;
    while (c != '{' && c != ';')
    {
        if(i == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if ( c == '\n' || c == '#' || std::isspace(c))
            if((i = consumeNonToken(ifs, c)) == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (c == '{' || c == ';')
            break ;

        arg = c;
        while (!std::isspace(c))
        {
            i = utils::get(ifs, c);
            if (i == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveBlock));
            if (c == '{' || c == ';' || c == '#'
                    || c == '\n' || std::isspace(c))
                break ;
            arg += c;
        }
        args.push_back(arg);
    }

    cfg::d_table::iterator      it;

    if ((it = directiveTable.find(args[0])) == directiveTable.end())
        if ((it = directiveTable.find(args[0])) == directiveTable.end())
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (it->second == cfg::D_BLOCK)
    {
        idxBlock++;
        idxSimple = 0;
        directivePool[idxBlock].name = args[0];
        directivePool[idxBlock].args.insert(
                directivePool[idxBlock].args.end(),
                args.begin() + 1, args.end());
        stt_parseDirective(ifs, c);
    }
    else if (it->second == cfg::D_SIMPLE)
    {
        idxSimple++;
        directivePool[idxBlock].dSimple[idxSimple].name = args[0];
        directivePool[idxBlock].dSimple[idxSimple].args.insert(
                directivePool[idxBlock].dSimple[idxSimple].args.end(),
                args.begin() + 1, args.end());
        stt_parseDirective(ifs, c);
    }
    else
        throw(static_cast<int>(cfg::err_BadDirective));
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

    /*while (1)
      {
      stt_parseDirective(ifs, c);
      if (ifs.peek() == EOF)
      break ;
      }*/

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

