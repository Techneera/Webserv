#include "utils.hpp"

// 1 - Assigns next char to c
// 2 - Returns next char through c
int utils::peekAndGet(std::ifstream &ifstream, char &c)
{
    if (!ifstream.get(c))
        return (EOF);

    return (c);
}

// Responsability: Consumes Spaces of a ifstream
int utils::consumeSpaces(std::ifstream &ifs, char &c)
{
    int i;

    while ((i = std::isspace(peekAndGet(ifs,c))));

    return (i);
}

// Responsability: Consumes Comments of a ifstream
int utils::consumeComments(std::ifstream &ifs, char &c)
{
    int     i;

    while ((i = utils::peekAndGet(ifs, c)) != '\n')
        if (i != EOF)
            break ;

    return (i);
}

int utils::consumeNewLines(std::ifstream &ifs, char &c)
{
    int i;

    while ((i = utils::peekAndGet(ifs, c)) == '\n');

    return (i);
}
