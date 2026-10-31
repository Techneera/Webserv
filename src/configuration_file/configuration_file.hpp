#pragma once

// INCLUDES ----------------------------------------------
//
#include <map>
#include <iostream>

namespace cfg
{
    // DEFINES -------------------------------------------
    // Max
    #define MAX_S_DIRECTIVE_SIZE 99
    #define MEMORY_POOL_SIZE 99

    // TYPES ---------------------------------------------
    // Errors enums
    enum    e_errors
    {
        err_FileOpening = 0,
        err_BadDirective = 1,
        err_BadDirectiveSimple = 2,
        err_BadDirectiveBlock = 3,
    };
    //
    // Directives names and enums
    enum    d_type
    {
        D_SIMPLE = 0,
        D_BLOCK = 1,
    };
    //
    // Directives, simple and block
    // TODO:
    //      1- Maybe assign  enums instead of names
    typedef struct d_simple
    {
        std::string name;
        std::string value;
    }   D_Simple;

    // Directive Look Up table
    typedef std::map<std::string, d_type>  d_table;

    // TODO:
    //      1- Review how a d_block will hold the other
    //          directives that are inside it. This might
    //          be overcomplicating it a lot since the 
    //          values will always be to feed the server
    //          class, and most probably nothing else
    //      2 - Make d_block maybe be a class with destructor
    //          and constructor, but destructor at least
    //          so its easier to clean it
    typedef struct d_block
    {
        bool            free;
        d_type          type;
        std::string     name;
        std::string     argument;
        D_Simple        dSimple[MAX_S_DIRECTIVE_SIZE];
        struct d_block  *next;
    }   D_Block;

    // METHODS --------------------------------------------
    // Functions
    int     fetch_config(char *filename);
    void    errorPrint(int code);
    void    configuration_file_init();
}
