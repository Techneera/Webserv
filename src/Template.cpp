#include "Template.hpp"

Template::Template()
{
    _buffer.n = 0;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++)
        _buffer.arr[i] = 0;
}

Template::~Template() {}

Template::Template(const Template& other)
{
    _buffer = other._buffer;
}

Template& Template::operator=(const Template& other)
{
    if (this != &other)
        _buffer = other._buffer;
    return *this;
}
