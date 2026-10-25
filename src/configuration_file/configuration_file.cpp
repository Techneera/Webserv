#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int is_valid_comment;

// TODO:
//      1- send this function to a more broader utils module
//      OR
//      1- create a wrapper class of std::ifstream
char    peekAndGet(std::ifstream &ifstream, char &c)
{
    ifstream.get(c);
    return (c);
}

/* TODO:
 *      1 - Maybe chagne to a better name after group discussion */
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
                    break ;

        consume_emptylines:
            // Consume empty lines
            while (c == '\n')
                if (peekAndGet(ifs, c) == EOF)
                    break ;

            // Consume comments
            if (c == '#')
            {
                if (peekAndGet(ifs, c) == EOF)
                    break ;
                goto consume_emptylines;
            }

        if (std::isspace(c) || c == '\n' || c == '#')
            goto consume_spaces;

        if (std::isdigit(static_cast<unsigned char>(c)))
        {
            digits += c;
            started = true;
        }
        else if (started)
            break;
    }

    if (digits.empty())
    {
        std::cerr << "ERROR: No port number foung in config"
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

