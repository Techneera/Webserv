#pragma once

// INCLUDES ----------------------------------------------
#include <iostream>
#include <map>
#include <vector>

#include "utils.hpp"

// @name:   namespace cfg
// @brief:  A namespace that defines several methods(functions,
//              typedefs..) for the configuration file module.
//          The configuration module aims to implement a parser
//              that will provide the necessary structured
//              data for the instantiation of the server class
namespace cfg
{
    // DEFINES -------------------------------------------


    // ---------------------------------------------------
    // TYPES ---------------------------------------------

    // @brief:  A simple enum for easy of use when checking
    //              the type of directive that is found
    //              on a look up table
    enum    d_type
    {
        D_SIMPLE = 0,
        D_BLOCK = 1,
    };

    // @brief:  std::map based look up table that serves
    //              as a quick way to validate any directive
    //              found on the configuration file
    typedef std::map<std::string, d_type>  d_table;

    // @brief:  Holds the Simple Directive data
    typedef struct d_simple
    {
        std::string     name;
        utl::sVector  args;
    }   D_Simple;

    // @brief:  Holds the Block Directive data which contains
    //              both Simple and Block Directives
    typedef struct d_block
    {
        std::string             name;
        utl::sVector          args;
        std::vector<D_Simple>   dSimple;
        std::vector<d_block>    dBlock;
    }   D_Block;


    //--------------------------------------------------------------
    // FUNCTIONS    ------------------------------------------------

    // @brief:  opens the configuration file and calls it's parser function
    //
    // @PARAM:  filename - webserver configuration file path
    //
    // @return: vector<cfg::D_Block>, an array of multiple D_Blocks that
    //              holds the server blocks which will provide the server
    //              class the necessary information for its instantiations
    std::vector<cfg::D_Block>   fetch_config(char *filename);

    // @brief:  Provides a specific error for any try catch block
    //              that catches a throw from the cfg namespace
    //
    // @PARAM:  code - the error code to determine error message
    //
    // return:  void
    void                        errorPrint(int code);;

    // @brief:  opens the configuration file and calls it's parser function
    //
    // @PARAM:  filename - webserver configuration file path
    //
    // @return: vector<cfg::D_Block>, an array of multiple D_Blocks that
    //              holds the server blocks which will provide the server
    //              class the necessary information for its instantiations
    void                        configuration_file_init();
}
