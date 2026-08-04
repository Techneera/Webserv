#include "ConfigParser.hpp"

// Default Constructor
ConfigParser::ConfigParser()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CONFIG_PARSER);
}

// Copy Constructor
ConfigParser::ConfigParser(const ConfigParser& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CONFIG_PARSER);
    *this = other;
}

// Copy Assignment operator
ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(CONFIG_PARSER);
    return *this;
}

// Destructor
ConfigParser::~ConfigParser()
{
    std::cout << DESTRUCTOR_MSG(CONFIG_PARSER);
}

