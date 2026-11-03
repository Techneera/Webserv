#include "server.hpp"

// Default Constructor
Server::Server()
{
    std::cout << CONSTRUCTOR_DEFAULT_MSG(CLASS);

    this->port = 0;
    this->region = "Unknown";
    this->host = "Unknown";
    this->errorPages = "Unknown";
    this->clientMaxBodySize = "Unknown";
}

// Copy Constructor
Server::Server(const Server& other)
{
    std::cout << CONSTRUCTOR_COPY_MSG(CLASS);
    this->port = other.port;
    this->region = other.region;
    this->host = other.host;
    this->errorPages = other.errorPages;
    this->location = other.location;
    this->clientMaxBodySize = other.clientMaxBodySize;
}

// Copy Assignment operator
Server& Server::operator=(const Server& other)
{
    std::cout << ASSIGNMENT_OPERATOR_COPY_MSG(CLASS);
    if (this != &other)
    {
        this->port = other.port;
        this->region = other.region;
        this->host = other.host;
        this->errorPages = other.errorPages;
        this->location = other.location;
        this->clientMaxBodySize = other.clientMaxBodySize;
    }
    return (*this);
}

// Destructor
Server::~Server()
{
    std::cout << DESTRUCTOR_MSG(CLASS);
}

Server::Server(size_t port, std::string region, std::string host,
        std::string errorPages, std::string clientMaxBodySize,
        sMap location)
{
    this->port = port;
    this->region = region;
    this->host = host;
    this->errorPages = errorPages;
    this->location = location;
    this->clientMaxBodySize = clientMaxBodySize;
}

// TO TRY TO USE THIS CODE RAFA IMPLEMENTED in the correct place
/*std::vector<cfg::D_Block>  cfg::fetch_config(char *filename)
{
    cfg::configuration_file_init();

    std::ifstream   ifs(filename);
    if (!ifs)
    {
        std::cerr << "ERROR: Cannot open config file: "
            << filename << std::endl;
        throw (static_cast<int>(err_FileOpening));
    }

    std::string     digits;
    char            c;

    return (stt_parseConfig(ifs, c));

    //bool          started;

    // (RAFA CODE) TODO: 
    //      1- Move to another function that parses ports
    if (std::isdigit(static_cast<unsigned char>(c)))
      {
      digits += c;
      started = true;
      }
      else if (started)
      break; */

    // (RAFA CODE) TODO: 
    //      1- Move to another function that parses ports
    /*if (digits.empty())
      {
      std::cerr << "ERROR: No port number found in config"
      << std::endl;
      return (-1);
      }

      errno = 0;
      char    *end = NULL;
      long    port = std::strtol(digits.c_str(), &end, 10);

      if (errno == ERANGE)
      {
      std::cerr << "ERROR: Port number out of range"
      << std::endl;
      return (-1);
      }
      if (end == digits.c_str() || *end != '\0')
      {
      std::cerr << "ERROR: Invalid port number" << std::endl;
      return (-1);
      }
      if (port < 1025 || port > 65535)
      {
      std::cerr << "ERROR: Port number must be "
      << "between 1025 and 65535" << std::endl;
      return (-1);
      }

      return (static_cast<int>(port));*/
}
