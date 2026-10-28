#pragma once

// INCLUDES -------------------------------------------------
//
#include <map>
#include <iostream>

namespace cfg
{
    // DEFINES --------------------------------------------------
    //
    #define MAX_S_DIRECTIVE_SIZE 99


    // TYPES ----------------------------------------------------
    //
    // Errors enums
    enum    e_errors
    {
        err_BadDirective = 1,
    };
    //
    // Directives names and enums
    enum    cf_directive
    {
    }   E_Directive;
    //
    // Directives, simple and block
    // TODO:
    //      1- Maybe assign  enums instead of names
    typedef struct d_simple
    {
        std::string name;
        std::string value;
    }   D_Simple;

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
        std::string     name;
        D_Simple        dSimple[MAX_S_DIRECTIVE_SIZE];
        struct d_block  *next;
    }   D_Block;

    // METHODS --------------------------------------------------
    // Variables

    // Functions
    int     fetch_config(char *filename);
    void    errorPrint(int code);
}
