#pragma once

#include <iostream>
#include <map>

#include "configuration_file.hpp"

#define CONFIG "Config"

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

namespace srv
{

    enum    e_region
    {
        e_portugal = 8000,
        e_brasil = 8001,
    };

    class Config
    {
        public:
            size_t          port; // port number
            std::string     ip; // ip address string
            size_t          clientMaxBodySize; // request max size
            std::string     errorPages; // directory of error pages
            sMap            location; // to find the directory of a certain page content
                                      //

            /* ORTHODOX CANONICAL FORM */
            // Default Constructor
            Config();
            // Copy Constructor
            Config(const Config& other);
            // Copy Assignment Operator
            Config& operator=(const Config& other);
            // Destructor
            ~Config();

            /* CONSTRUCTORS */
            Config(const cfg::D_Block &server);
    };
}
