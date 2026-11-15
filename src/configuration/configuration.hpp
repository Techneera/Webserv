#pragma once

// INCLUDES ----------------------------------------------
#include <iostream>
#include <vector>
#include <stdint.h>

#include "utils.hpp"


#define CONFIG "Config"

#ifndef MSG_OCF
#define MSG_OCF
#if MSG_LOG == 1
#define CONSTRUCTOR_DEFAULT_MSG(object) object
            << " Default constructor called\n"
#define CONSTRUCTOR_COPY_MSG(object) object
            << " Copy constructor called\n"
#define DESTRUCTOR_MSG(object) object << " Destructor called\n"
#define ASSIGNMENT_OPERATOR_COPY_MSG(object) object
            << " Copy assignment operator called\n"
#define MEMBER_FUNCTION_MSG(function) function
            << " member function called\n"
#else
#define CONSTRUCTOR_DEFAULT_MSG(object) ""
#define CONSTRUCTOR_COPY_MSG(object) ""
#define DESTRUCTOR_MSG(object) ""
#define ASSIGNMENT_OPERATOR_COPY_MSG(object) ""
#define MEMBER_FUNCTION_MSG(function) ""
#endif
#endif

// @name:   namespace cfg
// @brief:  A namespace that defines several methods(functions,
//              typedefs..) for the configuration file module.
//          The configuration module aims to implement a parser
//              that will provide the necessary structured
//              data for the instantiation of the server class
namespace cfg
{
    // DEFINES -------------------------------------------
    // @brief:  expected number of error code pages 
    #define ERROR_PAGES_SIZE 40

    // ---------------------------------------------------
    // TYPES ---------------------------------------------
    //
    // @brief:  
    enum    e_region
    {
        e_portugal = 8000,
        e_brasil = 8001,
    };

    // @brief:  A simple enum for easy of use when checking
    //              the type of directive that is found
    //              on a look up table
    //          It allows for a later O(1) speed of both assigning
    //              and checking the mandatory existence of
    //              necessary server configurations
    enum    e_directive_type : size_t
    {
        // Base directive types
        e_dir_simple = 1 << 1,
        e_dir_block = 1 << 2,

        // Simple directive subtypes
        e_listen = e_dir_simple | 1 << 3,
        e_host = e_dir_simple | 1 << 4,
        e_client_max_body_size = e_dir_simple | 1 << 5,
        e_error_pages = e_dir_simple | 1 << 6,
        e_root = e_dir_simple | 1 << 7,
        e_rust = e_dir_simple | 1 << 8,

        // Block directive subtypes
        e_http = e_dir_block | 1 << 9,
        e_location = e_dir_block | 1 << 11,
        e_cgi = e_dir_block | 1 << 12,
        e_server = e_dir_block | 1 << 15,
    };

    // @brief:  std::map based look up table that serves
    //              as a quick way to validate any directive
    //              found on the configuration file
    typedef std::map<std::string, e_directive_type>  map_dTable;

    // @brief:  Holds the Simple Directive data
    typedef std::multimap<e_directive_type, utl::sVector> mmap_dSimple;

    // @brief:  Holds the Block Directive data which contains
    //              both Simple and Block Directives
    typedef struct s_dBlock
    {
        std::string                                 name;
        utl::sVector                                args;
        mmap_dSimple                                dSimple;
        std::multimap<e_directive_type, s_dBlock>   dBlock;
    }   s_dBlock;

    //@brief: Holds the Directive Block data
    typedef std::multimap<e_directive_type, s_dBlock> mmap_dBlock;

    // @brief:  configuration struct that holds, semantically
    //              parsed, the information of a single server
    typedef struct s_config
    {
        size_t          port; // port number
        std::string     ip; // ip address string
        size_t          clientMaxBodySize; // request max size
        utl::lsMap      errorPages; // directory of error pages
        utl::lsMap      location; // to find the directory of a
                                  // certain page content
    }   s_config;

    //--------------------------------------------------------------
    // FUNCTIONS    ------------------------------------------------

    // @brief:  opens the configuration file and calls it's parser function
    //
    // @PARAM:  filename - webserver configuration file path
    //
    // @return: vector<cfg::s_config>
    std::vector<s_config>   fetch_config(char *filename);
}
