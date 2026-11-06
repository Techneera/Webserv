#pragma once

// INCLUDES ----------------------------------------------
#include <iostream>
#include <map>
#include <vector>
#include <stdint.h>

#include "utils.hpp"

// @name:   namespace cfg
// @brief:  A namespace that defines several methods(functions,
//              typedefs..) for the configuration file module.
//          The configuration module aims to implement a parser
//              that will provide the necessary structured
//              data for the instantiation of the server class
namespace cfg
{ // DEFINES -------------------------------------------


    // ---------------------------------------------------
    // TYPES ---------------------------------------------

    // @brief:  A simple enum for easy of use when checking
    //              the type of directive that is found
    //              on a look up table
    //          It allows for a later O(1) speed of both assigning
    //              and checking the mandatory existence of
    //              necessary server configurations
    enum    e_directive_type : uint16_t
    {
        // Base directive types
        e_dir_simple = 1 << 1,
        e_dir_block = 1 << 2,

        // Simple directive subtypes
        e_port = e_dir_simple | 1 << 3,
        e_listen = e_dir_simple | 1 << 4,
        e_region = e_dir_simple | 1 << 5,
        e_host = e_dir_simple | 1 << 6,
        e_client_max_body_size = e_dir_simple | 1 << 7,
        e_error_pages = e_dir_simple | 1 << 8,
        e_rust = e_dir_simple | 1 << 9,
        e_root = e_dir_simple | 1 << 10,

        // Block directive subtypes
        e_server = e_dir_block | 1 << 11,
        e_location = e_dir_block | 1 << 12,
        e_cgi = e_dir_block | 1 << 13,
    };

    // @brief:  std::map based look up table that serves
    //              as a quick way to validate any directive
    //              found on the configuration file
    typedef std::map<std::string, e_directive_type>  map_drtTable;

    // @brief:  Holds the Simple Directive data
    typedef std::map<e_directive_type, utl::sVector> map_drtSimple;

    // @brief:  Holds the Block Directive data which contains
    //              both Simple and Block Directives
    typedef struct d_block
    {
        std::string                         name;
        utl::sVector                        args;
        map_drtSimple                       dSimple;
        std::map<e_directive_type, d_block> dBlock;
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
}
