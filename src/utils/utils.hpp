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
        // Configuration File Module
        err_FileOpening,
        err_BadConfig,
        err_BadDirective,
        err_BadDirectiveSimple,
        err_BadDirectiveBlock,
        err_BigConfig,
        // Server Module
        err_BadPort,
        err_BadPortRange,
        err_BadIp,
        err_BadClientMaxBodySize,
        err_BadErrorPages,
        err_BadErrorPagesDir,
        err_BadErrorPagesAmount,
        err_MissingPort,
        err_MissingIp,
        err_MissingErrorPages,
        err_MissingClientMaxBodySize,
        err_MissingLocation,
    };
    typedef std::vector<std::string>            sVector;
    typedef sVector::iterator                   sVecIt;
    typedef std::map<std::string, std::string>  ssMap;
    typedef std::map<long, std::string>         lsMap;

    void    errorPrint(int &error);
};
