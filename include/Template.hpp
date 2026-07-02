#ifndef TEMPLATE_HPP
#define TEMPLATE_HPP

#include "DataModel.hpp"

class Template 
{
    chunck_t    _buffer;

    public:
    Template();
    ~Template();
    Template(const Template& other);
    Template&   operator=(const Template& other);

};

#endif
