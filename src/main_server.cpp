#include <cstring>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "configuration.hpp"
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

    cfg::D_Block        http = cfg::fetch_config(argv[1]);
    std::vector<Server> servers;
    std::map<cfg::e_directive_type, std::vector<cfg::D_Block> >::iterator   it;

    if ((it = http.dBlock.find(cfg::e_server)) == http.dBlock.end())
        exit(1);

    if (http.dBlock.size() != 1)
        exit(1);

    std::vector<cfg::D_Block>   configs = it->second;

    for (size_t i = 0; i < configs.size(); i++)
    {
        Server server(configs[i]);

        servers.push_back(server);
    }

  }
  catch (int &error)
  {
      utl::errorPrint(error);
      exit(error);
  }
}
