#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.hpp"
#include "configuration_file.hpp"

// @brief:  Look Up table to find if the directive is a valid one
static cfg::d_table directiveTable;

// @brief:  Populates the Look Up table as c++98 doesn't allow
//                  Initialization of a map container
//
// @PARAM:  none
//
// @return: void
void    cfg::configuration_file_init()
{
    directiveTable["listen"] = D_SIMPLE;
    directiveTable["region"] = D_SIMPLE;
    directiveTable["host"] = D_SIMPLE;
    directiveTable["port"] = D_SIMPLE;
    directiveTable["client_max_body_size"] = D_SIMPLE;
    directiveTable["root"] = D_SIMPLE;
    directiveTable["error_pages"] = D_SIMPLE;
    directiveTable[".rs"] = D_SIMPLE;

    directiveTable["server"] = D_BLOCK;
    directiveTable["location"] = D_BLOCK;
    directiveTable["cgi"] = D_BLOCK;
}

// @brief:  Provides a specific error for any try catch block
//              that catches a throw from the cfg namespace
//
// @PARAM:  code - the error code to determine error message
//
// return:  void
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
            break ;
        case err_BadDirectiveBlock:
            std::cout << "Bad block directive structure!\n";
            break ;
        case err_BadConfig:
            std::cout << "Expecting block directive!\n";
            break ;
        case err_BigConfig:
            std::cout << "Configuration file too big!\n";
            break ;
        default:
            std::cout << "Unknown cause!\n";
            break ;
    }
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
utils::sVector stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
{
    std::string     arg;
    utils::sVector  args(0);

    while (1)
    {
        if (c == '#' || c == '\n' || c == '}' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if (stt_consumeSpaces(ifs, c) == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if (c == '#' || c == '\n' || c == ';' || c == '}'
                || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        if (stt_parseToken(ifs, c, arg) == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

        args.push_back(arg);

        if (c == '}' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveSimple));

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
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));

    if (!ifs.get(c))
        throw (static_cast<int>(cfg::err_BadDirectiveBlock));


    directiveBlock.name = _name;

    std::string     name;

    while (c != '}')
    {
        if (stt_consumeNonToken(ifs, c) == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (c == '}')
        {
            stt_get(ifs, c);
            break ;
        }

        if (c == ';' || c == '{')
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (stt_parseToken(ifs, c, name) == EOF)
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        cfg::d_table::iterator      it;

        if ((it = directiveTable.find(name)) == directiveTable.end())
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));

        if (it->second == cfg::D_BLOCK)
        {
            if (stt_consumeNonToken(ifs, c) == EOF)
                throw (static_cast<int>(cfg::err_BadDirectiveBlock));

            if (name == "location")
            {
                if (stt_parseToken(ifs, c, name) == EOF)
                    throw (static_cast<int>(cfg::err_BadDirectiveBlock));

                if (stt_consumeNonToken(ifs, c) == EOF)
                    throw (static_cast<int>(cfg::err_BadDirectiveBlock));
            }

            directiveBlock.dBlock.push_back(
                            stt_parseDirectiveBlock(name, ifs, c));
        }
        else if(it->second == cfg::D_SIMPLE)
        {
            cfg::D_Simple    directiveSimple;

            directiveSimple.name = name;
            directiveSimple.args = stt_parseDirectiveSimple(ifs, c);
            directiveBlock.dSimple.push_back(directiveSimple);
        }
        else
            throw (static_cast<int>(cfg::err_BadDirectiveBlock));
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
            throw (static_cast<int>(cfg::err_BadConfig));

        if (stt_parseToken(ifs, c, arg) == EOF)
            throw (static_cast<int>(cfg::err_BadConfig));

        cfg::d_table::iterator      it;

        if (arg != "server")
            throw (static_cast<int>(cfg::err_BadConfig));

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

    return (stt_parseConfig(ifs, c));
}
