#include <cstring>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.hpp"
#include "configuration_file.hpp"

static int stt_parseDirectiveSimple()
{
    return (0);
}

// TODO:
//      1- Blocks contain Directives
//      2- Blocks start with '{' and end with '}'
static int stt_parseDirectiveBlock()
{
    return (0);
}

 // 1 Iterate untill a space to get the Directive Name
 // 2 Decide if Directive name corresponds to a simple
 //      or block directive
 //      or exit
 // 3 Decide if the directiveName will and how will it
 //   be saved
 // TODO: 
 //      1 - Maybe it makes sense to create a specific data
 //         structure to save all the information, since when 
 //         the directive name is valid, it wont return the 
 //         result until the whole directive is parsed.
 //         Maybe it makes sense to build a whole directive block
 //         data strucutre and the directive itself is the tree
static cfg::D_Block *stt_parseDirective(std::ifstream &ifs, char &c)
{
    cfg::D_Block *directive; // Will the new D_block keyworkd allow me
                             // to simplify the way I 'delete' it??
                             // Maybe I should crate the destructor then
    std::string directiveName = "";
    while (std::isalpha(utils::peekAndGet(ifs, c)))
        directiveName += c;

    if (!std::isspace(c))
        return (NULL);

    // CONTINUE FROM HERE

    return (directive);
}

// TODO:
//      1 - Maybe change to a better name after group discussion
//      2 - Understand when to use the consumers as sometimes
//          the spaces are meaninful, for example, when trying
//          to understand what's the name of the directive
int  fetch_config(char *filename)
{
    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        return (-1);
    }

    cfg::D_Block         *directive;
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
            directive = stt_parseDirective(ifs, c);
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

