#pragma once

// INCLUDES -------------------------------------------------
#include <map>
#include <iostream>


// TYPES ----------------------------------------------------
typedef int         port;
typedef std::string address;
typedef std::string directory;
typedef std::map<address, directory>    configKeyValue;
typedef std::map<port, configKeyValue>  configMap;
typedef configMap::iterator                 configMapIt;
typedef configMap::const_iterator           const_configMapIt;


// METHODS --------------------------------------------------
int  fetch_config(char *filename);
