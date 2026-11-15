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

class Server
{
    public:
        // @brief:  Amount of unique error pages that needs to exist
        cfg::s_config       config;
        int                 serverSocket;

        /* ORTHODOX CANONICAL FORM */
        // Default Constructor
        Server();
        // Copy Constructor
        Server(const Server& other);
        // Copy Assignment Operator
        Server& operator=(const Server& other);
        // Destructor
        ~Server();

        Server(const cfg::s_config &dBlock);
};
