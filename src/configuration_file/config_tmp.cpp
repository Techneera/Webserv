#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int  fetch_config(char *filename)
{
    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: " << filename << std::endl;
        return (-1);
    }

    std::string     digits;
    char            c;
    bool            started;

    while (ifs.get(c))
    {
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
        std::cerr << "ERROR: No port number foung in config" << std::endl;
        return (-1);
    }

    errno = 0;
    char    *end = NULL;
    long    port = std::strtol(digits.c_str(), &end, 10);

    if (errno == ERANGE)
    {
        std::cerr << "ERROR: Port number out of range" << std::endl;
        return (-1);
    }
    if (end == digits.c_str() || *end != '\0')
    {
        std::cerr << "ERROR: Invalid port number" << std::endl;
        return (-1);
    }
    if (port < 1025 || port > 65535)
    {
        std::cerr << "ERROR: Port number must be between 1025 and 65535" << std::endl;
        return (-1);
    }

    return (static_cast<int>(port));
}


