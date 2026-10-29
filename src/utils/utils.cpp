#include "utils.hpp"

// 1 - Assigns next char to c
// 2 - Returns next char through c
int utils::peekAndGet(std::ifstream &ifstream, char &c)
{
    if (!ifstream.get(c))
        return (EOF);
    return (c);
}

int utils::consumeSpaces(std::ifstream &ifs, char &c)
{
    while (std::isspace(c))
        if (utils::peekAndGet(ifs, c) == EOF)
            return (EOF) ;

    return (0);
}

int utils::consumeComments(std::ifstream &ifs, char &c)
{
    if (c == '#')
    {
        while (utils::peekAndGet(ifs, c) != EOF)
            if (c == '\n')
                return (c);
    }

    return (c);
}
