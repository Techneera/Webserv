#include "response.hpp"

// Default Constructor
Response::Response()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(RESPONSE);
}

// Copy Constructor
Response::Response(const Response& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(RESPONSE);
    *this = other;
}

// Copy Assignment operator
Response& Response::operator=(const Response& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(RESPONSE);
    return *this;
}

// Destructor
Response::~Response()
{
    std::cout << DESTRUCTOR_MSG(RESPONSE);
}

