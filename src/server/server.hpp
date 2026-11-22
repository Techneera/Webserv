#pragma once

#include <iostream>

#include "configuration.hpp"

#define SERVER "server"

#ifndef MSG_OCF
#define MSG_OCF
#if MSG_LOG == 1
#define CONSTRUCTOR_DEFAULT_MSG(object) object << " Default constructor called\n"
#define CONSTRUCTOR_COPY_MSG(object) object << " Copy constructor called\n"
#define DESTRUCTOR_MSG(object) object << " Destructor called\n"
#define ASSIGNMENT_OPERATOR_COPY_MSG(object) object << " Copy assignment operator called\n"
#define MEMBER_FUNCTION_MSG(function) function << " member function called\n"
#else
#define CONSTRUCTOR_DEFAULT_MSG(object) ""
#define CONSTRUCTOR_COPY_MSG(object) ""
#define DESTRUCTOR_MSG(object) ""
#define ASSIGNMENT_OPERATOR_COPY_MSG(object) ""
#define MEMBER_FUNCTION_MSG(function) ""
#endif
#endif

// @brief:  configuration struct that hold the parses information of
//              a single server
typedef struct config
{
    size_t          port; // port number
    std::string     ip; // ip address string
    size_t          clientMaxBodySize; // request max size
    utl::lsMap      errorPages; // directory of error pages
    utl::lsMap      location; // to find the directory of a
                              // certain page content
}   Config;

class Server
{
    public:
        // @brief:  Amount of unique error pages that needs to exist
        static const size_t _errorCodesNo;
        Config              config;
        int                 serverSocket;
        int                 clientSocket;

        /* ORTHODOX CANONICAL FORM */
        // Default Constructor
        Server();
        // Copy Constructor
        Server(const Server& other);
        // Copy Assignment Operator
        Server& operator=(const Server& other);
        // Destructor
        ~Server();

        Server(const cfg::D_Block &dBlock);
};
