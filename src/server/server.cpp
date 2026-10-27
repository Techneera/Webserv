#include "Class.hpp"

// Default Constructor
Class::Class()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CLASS);
}

// Copy Constructor
Class::Class(const Class& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CLASS);
    *this = other;
}

// Copy Assignment operator
Class& Class::operator=(const Class& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(CLASS);
    return *this;
}

// Destructor
Class::~Class()
{
    std::cout << DESTRUCTOR_MSG(CLASS);
}

