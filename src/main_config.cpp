#include <cstring>
#include <iostream>

#include "configuration_file.hpp"

int main(int argc, char **argv)
{
  if (argc != 2)
  {
      std::cerr << "ERROR: Usage: " << argv[0]
                << " <config_file>" << std::endl;
      return (1);
  }
  std::cout << "Setting " << argv[1]
            << " as the configuration file\n";

  // TODO:
  //        1 - Create config file parser function
  //        2 - Maybe give it struct address to populate with 
  //            necessary instructions/info suchs as port,
  //            or file dir
  std::vector<cfg::D_Block>  config;

  try
  {
    config = cfg::fetch_config(argv[1]);
  }
  catch (int &error)
  {
      cfg::errorPrint(error);
  }

  // TODO: insert this Rafael code in the server module
  /*if (port < 0)
  {
      std::cerr << "ERROR: Failed reading port from config"
                << std::endl;
      return (-1);
  }*/
}
