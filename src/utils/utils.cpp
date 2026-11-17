#include "utils.hpp"

namespace utl
{
    // @brief:  Provides a specific error for any try catch block
    //              that catches a throw from any module
    //
    // @PARAM:  code - the error code to determine error message
    //
    // return:  void
    void    errorPrint(int &code)
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
                std::cerr << "Port number out of range, must be"
                    << " between 1025 and 65535" << std::endl;
                break ;
            case err_BadIp:
                std::cerr << "Wrong ip directive!\n";
                break ;
            case err_BadClientMaxBodySize:
                std::cerr << "Wrong client_max_body_size directive!\n";
                break ;
            case err_BadErrorPages:
                std::cerr << "Wrong error_pages directive!\n";
                break ;
            case err_BadErrorPagesDir:
                std::cerr << "Cannot open error_pages directive!\n";
                break ;
            case err_BadErrorPagesAmount:
                std::cerr <<"Unexpected amount of error pages\n";
                break ;
            case err_BadHttpBlock:
                std::cerr <<"Expected server block\n";
                break ;
            case err_MissingPort:
                std::cerr <<"Missing Port from server configuration\n";
                break ;
            case err_MissingIp:
                std::cerr <<"Missing Ip from server configuration\n";
                break ;
            case err_MissingClientMaxBodySize:
                std::cerr <<"Missing ClientMaxBodySize from server configuration\n";
                break ;
            case err_MissingErrorPages:
                std::cerr <<"Missing ErrorPages from server configuration\n";
                break ;
            case err_MissingLocation:
                std::cerr <<"Missing Location from server configuration\n";
                break ;
            default:
                std::cout << "Unknown cause!\n";
                break ;
        }
    }


    int     fileToString(const std::string &filePath, std::string &str)
    {
        std::ifstream   ifsFilePath(filePath.c_str());

        if (!ifsFilePath)
            return (-1);

        ifsFilePath.seekg(0, std::ios::end);

        size_t      size = ifsFilePath.tellg();
        std::string buffer(size, ' ');

        ifsFilePath.seekg(0);
        ifsFilePath.read(&buffer[0], size);

        str = buffer;

        return (1);
    }

}

