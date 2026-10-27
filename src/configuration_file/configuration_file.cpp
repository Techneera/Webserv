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
    ifstream.get(c);
    return (c);
}

static int stt_parsePort()
{
    return (0);
}

// TODO:
//      1- Directives finish with ';'
static int stt_parseDirective()
{
    return (0);
}

// TODO:
//      1- Blocks contain Directives
//      2- Blocks start with '{' and end with '}'
static int stt_parseBlock()
{
    return (0);
}

// TODO:
//      1 - Maybe change to a better name after group discussion
//      2 - Consumer loops must be restructured into functions
//          due to the necessity of checking if the block or simple
//          directives are finished or not
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
    bool            started;

    while (ifs.get(c))
    {
        consume_spaces:
            // Consume spaces
            while (std::isspace(c))
                if (peekAndGet(ifs, c) == EOF)
                    return (-1) ;

        consume_emptylines:
            // Consume empty lines
            while (c == '\n')
                if (peekAndGet(ifs, c) == EOF)
                    return (1) ;

            // Consume comments
            if (c == '#')
            {
                if (peekAndGet(ifs, c) == EOF)
                    return (1) ;
                goto consume_emptylines;
            }

        if (std::isspace(c) || c == '\n' || c == '#')
            goto consume_spaces;

        // TODO:
        //      1- Move to another function that parses ports
        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            digits += c;
            started = true;
        }
        else if (started)
            break;
    }

    // TODO:
    //      1- Move to another function that parses ports
    if (digits.empty())
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

    return (static_cast<int>(port));

}

