#pragma once

#include <iostream>
#include <fstream>
#include <vector>

namespace   utils
{
    typedef std::vector<std::string>    sVector;
    typedef sVector::iterator           sVecIt;

    int peekAndGet(std::ifstream &ifstream, char &c);
    int consumeSpaces(std::ifstream &ifs, char &c);
    int consumeComments(std::ifstream &ifs, char &c);
    int consumeNewLines(std::ifstream &ifs, char &c);
    int isDigit();
};
