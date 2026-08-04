#pragma once

#include <iostream>

#define CONFIG_PARSER "ConfigParser"

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

class   ConfigParser
{
    private:

    public:
            /* ORTHODOX CANONICAL FORM */
            // Default Constructor
            ConfigParser();
            // Copy Constructor
            ConfigParser(const ConfigParser& other);
            // Copy Assignment Operator
            ConfigParser& operator=(const ConfigParser& other);
            // Destructor
            ~ConfigParser();
};

