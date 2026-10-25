#include "configuration_file.hpp"

#include <vector>

// TODO:
//      1- Messing around with maps to understand how
//          feasible it is to use them to hold the
//          configuration file information
//      2- Need to draw out a good structure for the
//          configuration file parsing
//      3- Theres a scenario where the http request parser
//          and the configuration file parser need to be
//          synced
int main(void)
{
    int             port;
    address         key;
    directory       value;
    configKeyValue  KV;
    configMap       configFile;

    key = "/";
    value = "assets/data/www";
    KV.insert({key, value});
    port = 8000;
    configFile.insert({port, KV});

    key = "/images";
    value = "assets/data/images/";
    KV.insert({key, value});
    port = 8000;
    configFile.insert({port, KV});

    try
    {
        std::cout << configFile.at(8000).at("/");
        std::cout << "\n";
    }
    catch (...)
    {
        std::cout << "404: Page Not Found\n";
        return (404);
    }

    try
    {
        std::cout << configFile.at(8000).at("/images");
        std::cout << "\n";
    }
    catch (...)
    {
        std::cout << "404: Page Not Found\n";
        return (404);
    }
}
