#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

namespace   utl
{

    // @brief:  Error number codes for throwing errors
    enum    e_errors
    {
        err_FileOpening,
        err_BadConfig,
        err_BadDirective,
        err_BadDirectiveSimple,
        err_BadDirectiveBlock,
        err_BigConfig,

        err_BadPort,
        err_BadPortRange,
    };
    typedef std::vector<std::string>    sVector;
    typedef sVector::iterator           sVecIt;

    void    errorPrint(int &error);
};
