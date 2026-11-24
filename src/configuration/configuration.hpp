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

    // @brief: error pages response template placeholders
    //          index
    #define STATUS_CODE_LEN 13
    #define REASON_PHRASE_LEN 15
    #define EXTRA_LINE_LEN 12
    #define AUTHENTICATION_CHALLENGE
    #define BODY_LENGTH_LEN 13
    #define BODY_LEN 6

    // ---------------------------------------------------
    // TYPES ---------------------------------------------
    // @brief:  internal module errors
    enum    e_configuration
    {
        e_wrongFilePath = -1,
    };

    // @brief:  region names, might be fun and useful as an ID
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
        e_methods = e_dir_simple | 1 << 9,
        e_return = e_dir_simple | 1 << 10,
        e_autoindex = e_dir_simple | 1 << 11,
        e_index = e_dir_simple | 1 << 12,
        e_upload_path = e_dir_simple | 1 << 13,

        // Block directive subtypes
        e_http = e_dir_block | 1 << 16,
        e_server = e_dir_block | 1 << 17,
        e_location = e_dir_block | 1 << 18,
        e_cgi = e_dir_block | 1 << 19,
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

    // @brief: Holds the Directive Block data
    typedef std::multimap<e_directive_type, s_dBlock> mmap_dBlock;

    // TODO:
    //      1- To define what type each block is. For now they are
    //          placeholders, maybe they are definitive, who knows, me? no, you?
    //          naaaaaah!
    //
    // @brief:  Representing the location block, holds multiple directives
    typedef struct s_location
    {
        std::string root;
        utl::ibMap  methods; // can be half assigned already, then off or on
        bool        autoIndex;
        std::string index; // maybe the actual cached response already?
        std::string uploadPath;
    }   s_location;

    // TODO:
    //      1- To verify if the map should be exactly like this
    //
    // @brief: Holds a hashtable for the several location blocks within
    //          a server block
    typedef std::multimap<long, s_location> mmap_location;

    // @brief:  configuration struct that holds, semantically
    //              parsed, the information of a single server
    typedef struct s_config
    {
        size_t          port; // port number
        std::string     ip; // ip address string
        size_t          clientMaxBodySize; // request max size
        utl::lsMap      errorPages; // directory of error pages
        mmap_location   location; // to find the correct location
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
