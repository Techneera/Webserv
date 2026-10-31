#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.hpp"
#include "configuration_file.hpp"

static int          poolIdx;
// TODO:
//      Might just not return the nodes on the parseDirective funcitons
//      and just throw the errors if the actual static allocation works
static cfg::D_Block directivePool[99];
static cfg::d_table directiveType;

void    cfg::configuration_file_init()
{
    directiveType["server"] = D_SIMPLE;
    directiveType["region"] = D_SIMPLE;
    directiveType["host"] = D_SIMPLE;
    directiveType["port"] = D_SIMPLE;
    directiveType["client_max_body_size"] = D_SIMPLE;
    directiveType["root"] = D_SIMPLE;
    directiveType["error_pages"] = D_SIMPLE;

    directiveType["server"] = D_BLOCK;
    directiveType["location"] = D_BLOCK;
    directiveType["cgi"] = D_BLOCK;
}

//      1- Blocks contain Directives
//      2- Blocks start with '{' and end with '}'
// TODO:
//      1- Figure out when to skip comments
static
cfg::D_Block    *stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
{
    return (NULL);
}


//      1 Iterate untill a space to get the Directive Name
//      2 Decide if Directive name corresponds to a simple
//           or block directive
//           or exit
//      3 Decide if the directiveName will and how will it
//        be saved
//      4- Blocks contain Directives
//      5- Blocks start with '{' and end with '}'
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
cfg::D_Block    *stt_parseDirectiveBlock(std::ifstream &ifs, char &c)
{
    cfg::d_table::iterator      it;
    bool                        newToken;
    int                         i;
    int                         argCount = 1;
    std::string                 directiveName;

    while (!(i = std::isspace(utils::peekAndGet(ifs, c))))
    {
        if (i == EOF)
            return NULL ;
        directiveName += c;
    }
    if ((it = directiveType.find(directiveName)) == directiveType.end())
        return  (NULL);
    if (directiveName == "location")
        argCount--;
    while ((i = (utils::peekAndGet(ifs, c))) != '{')
    {
        newToken = true;
        std::string argument = "";
        while (!(i = std::isspace(utils::peekAndGet(ifs, c))))
        {
            if (i == EOF)
                return NULL ;
            if (newToken == true)
            {
                argCount++;
                argument += c;
            }
            newToken = false;
        }
        if (argCount > 1)
            return (NULL);
    }

    if (argCount != 1)
        return NULL ;

    if (i == EOF)
        return (NULL);

    if (it->second == cfg::D_SIMPLE)
        return (stt_parseDirectiveSimple(ifs, c));
    if (it->second == cfg::D_BLOCK)
        return (stt_parseDirectiveBlock(ifs, c));

    if (c != '}')
        return NULL ;

    return (NULL); // return the actual block
}

// TODO:
//      1 - Maybe change to a better name after group discussion
//      2 - Understand when to use the consumers as sometimes
//          the spaces are meaninful, for example, when trying
//          to understand what's the name of the directive
int  cfg::fetch_config(char *filename)
{
    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        return (-1);
    }

    cfg::D_Block    *directive;
    std::string     digits;
    char            c;
    //bool            started;

    while (ifs.get(c))
    {
        // TODO: Will need to check for the comments at least
        if (std::isalpha(c))
        {
            // Try and parse name to then parse the whole
            // directive
            directive = stt_parseDirectiveBlock(ifs, c);
            if (directive == NULL)
                throw (cfg::err_BadDirective);
        }

        // (RAFA CODE) TODO: 
        //      1- Move to another function that parses ports
        /*if (std::isdigit(static_cast<unsigned char>(c)))
          {
          digits += c;
          started = true;
          }
          else if (started)
          break; */
    }

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
        default:
            std::cout << "unknown cause!\n";
            break ;
    }
}

