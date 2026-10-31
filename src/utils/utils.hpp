#pragma once

#include <iostream>
#include <fstream>

namespace   utils
{
    int peekAndGet(std::ifstream &ifstream, char &c);
    int consumeSpaces(std::ifstream &ifs, char &c);
    int consumeComments(std::ifstream &ifs, char &c);
    int consumeNewLines(std::ifstream &ifs, char &c);
    int isDigit();
};
