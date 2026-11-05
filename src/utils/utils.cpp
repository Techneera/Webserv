#include "utils.hpp"

// @brief:  Provides a specific error for any try catch block
//              that catches a throw from any module
//
// @PARAM:  code - the error code to determine error message
//
// return:  void
void    utl::errorPrint(int &code)
{
    std::cout << "Error: ";
    switch (code)
    {
        case err_BadDirective:
            std::cout << "Bad directive!\n";
            break ;
        case err_BadDirectiveSimple:
            std::cout << "Bad simple directive structure!\n";
            break ;
        case err_BadDirectiveBlock:
            std::cout << "Bad block directive structure!\n";
            break ;
        case err_BadConfig:
            std::cout << "Expecting block directive!\n";
            break ;
        case err_BigConfig:
            std::cout << "Configuration file too big!\n";
            break ;
        case err_BadPort:
            std::cout << "Wrong port directive!\n";
            break ;
        case err_BadPortRange:
            std::cerr << "ERROR: Port number out of range, must be"
                      << " between 1025 and 65535" << std::endl;
            break ;
        default:
            std::cout << "Unknown cause!\n";
            break ;
    }
}

