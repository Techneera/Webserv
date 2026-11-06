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

  try
  {

    std::vector<cfg::D_Block>   config = cfg::fetch_config(argv[1]);
    std::vector<srv::Server>    servers;

    for (size_t i = 0; i < config.size(); i++)
    {
        srv::Server server(config[i]);

        servers.push_back(server);
    }

  }
  catch (int &error)
  {
      utl::errorPrint(error);
      exit(error);
  }
}
