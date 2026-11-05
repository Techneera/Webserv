#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "configuration_file.hpp"
#include "server.hpp"

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

  std::vector<cfg::D_Block>  config;

  try
  {
    config = cfg::fetch_config(argv[1]);
  }
  catch (int &error)
  {
      utl::errorPrint(error);
      exit(error);
  }
}
