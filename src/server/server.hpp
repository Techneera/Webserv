#pragma once

#include <iostream>
#include <map>

#define SERVER "Server"

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

typedef std::map<std::string, std::string>  sMap;

class   Server
{
    public:
            size_t          port; // port number
            std::string     region; // just messing around, it's not needed, but it's cool
            std::string     host; // ip address string
            std::string     errorPages; // directory of error pages
            sMap            location; // to find the directory of a certain page content

            /* ORTHODOX CANONICAL FORM */
            // Default Constructor
            Server();
            // Copy Constructor
            Server(const Server& other);
            // Copy Assignment Operator
            Server& operator=(const Server& other);
            // Destructor
            ~Server();

            /* CONSTRUCTORS */
            Server(size_t port, std::string region, std::string host,
                   std::string errorPages, sMap location);
};

