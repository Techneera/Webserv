#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

// TODO:
//      1- send this function to a more broader utils module
//      OR
//      1- create a wrapper class of std::ifstream
int peekAndGet(std::ifstream &ifstream, char &c)
{
    if (!ifstream.get(c))
        return (EOF);
    return (c);
}

//TODO:
//      1- Check if, in case of EOF, the file ended correctly
int consumeSpaces(std::ifstream &ifs, char &c)
{
    while (std::isspace(c))
        if (peekAndGet(ifs, c) == EOF)
            return (EOF) ;

    return (0);
}

//TODO:
//      1- Check if, in case of EOF, the file ended correctly
int consumeComments(std::ifstream &ifs, char &c)
{
    if (c == '#')
    {
        while (peekAndGet(ifs, c) != EOF)
            if (c == '\n')
                return (c);
    }

    return (c);
}

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

/* 1 Iterate untill a space to get the Directive Name
 * 2 Decide if Directive name corresponds to a simple
 *      or block directive
 *      or exit */
static int stt_parseDirectiveName(std::ifstream &ifs, char &c)
{
    while (std::isspace(peekAndGet(ifs, c)))
    {
    }

    return (0);
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

    std::string     digits;
    char            c;
    //bool            started;

    while (ifs.get(c))
    {
        while (std::isspace(c) || c == '\n' || c == '#')
        {
            if (consumeSpaces(ifs, c) == EOF
                || consumeComments(ifs, c) == EOF)
                break ;
        }

        // TODO:
        //      1- Move to another function that parses ports
        /*if (std::isdigit(static_cast<unsigned char>(c)))
          {
          digits += c;
          started = true;
          }
          else if (started)
          break; */
    }

    // TODO:
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

