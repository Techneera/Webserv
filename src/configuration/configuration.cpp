#include <cstring>

#include <string>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sstream>

#include "utils.hpp"
#include "configuration.hpp"

namespace   cfg
{

    // @brief:  Look Up table to find if the directive is a valid one
    static map_dTable directiveTable;

    // TODO:
    //      1- Build the Generic Error response
    //      2- Build the other possibilities from there, if possible
    // @brief: Predefined template for a more direct cache
    //          approach
    const std::string  errorPageResponseTemplate =
                            "HTTP/1.1 <STATUS_CODE> <REASON_PHRASE>\r\n"
                            "<EXTRA_LINE>"
                            "Content-Type: text/html\r\n"
                            "Content-Length: <BODY_LENGTH>\r\n"
                            "Connection: close\r\n\r\n<BODY>";

    // @brief: Look Up table for status code and respective reason phrase
    static utl::lsMap   reasonPhraseTable;

    static utl::lsMap   extraLineTable;

    // @brief:  Populates the Look Up table as c++98 doesn't allow
    //                  Initialization of a map container
    //
    // @PARAM:  none
    //
    // @return: void
    static void    configuration_file_init()
    {
        directiveTable["listen"] = e_listen;
        directiveTable["host"] = e_host;
        directiveTable["client_max_body_size"] = e_client_max_body_size;
        directiveTable["error_pages"] = e_error_pages;
        directiveTable["root"] = e_root;
        directiveTable[".rs"] = e_rust;
        directiveTable["methods"] = e_methods;
        directiveTable["return"] = e_return;
        directiveTable["autoindex"] = e_autoindex;
        directiveTable["index"] = e_index;
        directiveTable["upload_path"] = e_upload_path;

        directiveTable["http"] = e_http;
        directiveTable["server"] = e_server;
        directiveTable["location"] = e_location;
        directiveTable["cgi"] = e_cgi;


        reasonPhraseTable[400] = "Bad Request";
        reasonPhraseTable[401] = "Unauthorized";
        reasonPhraseTable[402] = "Payment Required";
        reasonPhraseTable[403] = "Forbidden";
        reasonPhraseTable[404] = "Not Found";
        reasonPhraseTable[405] = "Method Not Allowed";
        reasonPhraseTable[406] = "Not Acceptable";
        reasonPhraseTable[407] = "Proxy Authentication Required";
        reasonPhraseTable[408] = "Request Timeout";
        reasonPhraseTable[409] = "Conflict";
        reasonPhraseTable[410] = "Gone";
        reasonPhraseTable[411] = "Length Required";
        reasonPhraseTable[412] = "Precondition Failed";
        reasonPhraseTable[413] = "Content Too Large";
        reasonPhraseTable[414] = "URI Too Long";
        reasonPhraseTable[415] = "Unsupported Media Type";
        reasonPhraseTable[416] = "Range Not Satisfiable";
        reasonPhraseTable[417] = "Expectation Failed";
        reasonPhraseTable[418] = "(Unused)";
        reasonPhraseTable[419] = "Unassigned"; // Non official
        reasonPhraseTable[421] = "Misdirected Request";
        reasonPhraseTable[422] = "Unprocessable Content";
        reasonPhraseTable[423] = "Locked";
        reasonPhraseTable[424] = "Failed Dependency";
        reasonPhraseTable[425] = "Too Early";
        reasonPhraseTable[426] = "Upgrade Required";
        reasonPhraseTable[428] = "Precondition Required";
        reasonPhraseTable[429] = "Too Many Requests";
        reasonPhraseTable[431] = "Request Header Fields Too Large";
        reasonPhraseTable[451] = "Unavailable For Legal Reasons";
        reasonPhraseTable[500] = "Internal Server Error";
        reasonPhraseTable[501] = "Not Implemented";
        reasonPhraseTable[502] = "Bad Gateway";
        reasonPhraseTable[504] = "Gateway Timeout";
        reasonPhraseTable[505] = "HTTP Version Not Supported";
        reasonPhraseTable[506] = "Variant Also Negotiates";
        reasonPhraseTable[507] = "Insufficient Storage";
        reasonPhraseTable[508] = "Loop Detected";
        reasonPhraseTable[510] = "Not Extended";
        reasonPhraseTable[511] = "Network Authentication Required";

        extraLineTable[401] = "WWW-Authenticate: "
                              "<RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
        extraLineTable[405] = "Allow: <RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
        extraLineTable[407] = "WWW-Authenticate: "
                              "<RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
        extraLineTable[416] = "Content-Range: "
                              "<RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
        extraLineTable[426] = "Upgrade: <RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
        extraLineTable[429] = "Retry-After: <RESPONSE_EXTRA_LINE_ARGUMENT>\r\n";
    }

    // @brief:  Provides a small ease of use modification of
    //              the standard ifstream method get()
    //
    // @PARAM:  ifstream - config file stream; c - holds next char
    //
    // @return: int (to be able to return both EOF or any char)
    static int stt_get(std::ifstream &ifstream, char &c)
    {
        if (!ifstream.get(c))
            return (EOF);

        return (c);
    }

    // @brief:  Consumes Spaces of a ifstream
    //
    // @PARAM:  ifstream - config file stream; c - holds next char
    //
    // @return: int (EOF or next char)
    static int stt_consumeSpaces(std::ifstream &ifs, char &c)
    {
        int i;

        while ((i = stt_get(ifs, c)) != EOF)
        {
            if (!std::isspace(c))
                break ;
        }

        return (i);
    }

    // @brief:  Consumes Comments of a ifstream
    //
    // @PARAM:  ifstream - config file stream; c - holds next char
    //
    // @return: int (EOF or next char)
    static int stt_consumeComments(std::ifstream &ifs, char &c)
    {
        int     i;

        while ((i = stt_get(ifs, c)) != '\n')
            if (i == EOF)
                break ;

        return (i);
    }

    // @brief:  Consumes new line chars of a ifstream
    //
    // @PARAM:  ifstream - config file stream; c - holds next char
    //
    // @return: int (EOF or next char)
    static int stt_consumeNewLines(std::ifstream &ifs, char &c)
    {
        int i;

        while ((i = stt_get(ifs, c)) == '\n');

        return (i);
    }

    // @brief:      Consumes any blocks of characters that are
    //                  not relevant for the webserver configuration
    //                  such as comments, new lines and spaces
    //
    // @PARAM:      ifstream - config file stream; c - holds next char
    //
    // @return:     int (EOF or next char)
    //
    // @details:    1 - Check which is the current char
    //              2 - If current char is '\n', consume any '\n' lines
    //              3 - If current char is '#', consume comment line
    //              4 - If current char is '<space>', consume spaces
    //              5 - Else,  it is the start of a token, so return char
    static int stt_consumeNonToken(std::ifstream &ifs, char &c)
    {
        if  (c != '\n' && c != '#' && !std::isspace(c))
            return  (c);

        while (1)
        {
            if (c == '\n')
            {
                if (stt_consumeNewLines(ifs, c) == EOF)
                    return (EOF);
            }
            else if (c == '#')
            {
                if (stt_consumeComments(ifs, c) == EOF)
                    return  (EOF);
            }
            else if (std::isspace(c))
            {
                if (stt_consumeSpaces(ifs, c) == EOF)
                    return  (EOF);
            }
            else
                break ;
        }

        return (1);
    }

    // @brief:  Saves next token in <token> param. A token is any
    //             block of the config file ifstream that is
    //             separated by spaces, is not a comment, is not
    //             a new line, EOF or any special character that
    //             determines the end or the beggining of a simple
    //             or block directive
    //
    // @PARAM:  ifstream - config file stream; c - holds next char;
    //             token - token to be saved
    //
    // @return: int (EOF or next char)
    static int stt_parseToken(std::ifstream &ifs, char &c, std::string &token)
    {
        token = "";

        while (c != '#' && c != '\n' && c != ';' && c != '}'
                && c != '{' && !std::isspace(c))
        {
            token += c;
            if (stt_get(ifs, c) == EOF)
                return EOF;
        }

        return (1);
    }

    // @brief:  parses ifstream until the char ';' which determines
    //              the end of a simple directive section
    //
    // @PARAM:  ifstream - config file stream; c - holds next char;
    //              token - token to be saved
    //
    // @return: vector<std::string> that holds the arguments that
    //              succeed the directive name
    //
    // @details:    1 - While the char ';' is not found
    //              2 -     Parse next token
    //              3 -     Add token to a tokens vector
    //              4 - Return the tokens vector
    static utl::sVector stt_parseDirectiveSimple(std::ifstream &ifs, char &c)
    {
        std::string     arg;
        utl::sVector  args(0);

        while (1)
        {
            if (c == '#' || c == '\n' || c == '}' || c == '{')
                throw (static_cast<int>(utl::err_BadDirectiveSimple));

            if (stt_consumeSpaces(ifs, c) == EOF)
                throw (static_cast<int>(utl::err_BadDirectiveSimple));

            if (c == '#' || c == '\n' || c == ';' || c == '}'
                    || c == '{')
                throw (static_cast<int>(utl::err_BadDirectiveSimple));

            if (stt_parseToken(ifs, c, arg) == EOF)
                throw (static_cast<int>(utl::err_BadDirectiveSimple));

            args.push_back(arg);

            if (c == '}' || c == '{')
                throw (static_cast<int>(utl::err_BadDirectiveSimple));

            if (c == ';')
            {
                stt_get(ifs, c);
                return (args);
            }
        }
    }

    // @brief:      parses ifstream until the char '}' which determines
    //                  the end of a Block Directive section. It provides
    //                  the parsing of nested Block Directives
    //
    // @PARAM:      ifstream - config file stream; c - holds next char;
    //                  token - token to be saved
    //
    // @return:     s_dBlock that holds the parsed Directive Block.
    //
    // @details:    1 - While the char '}' is not found
    //              2 -     Parse next token
    //              3 -     If token is Block Directive
    //              4 -         Set token as Block Directive name
    //              5 -         Parse Block Directive and save it in the current block
    //              6 -         directive structure
    //              7 -     Else If token is Simple Directive
    //              8 -         Set token as Simple Directive name
    //              9 -         Parse Simple Directive and save it in the current block
    //              a -         directive structure
    //              b -     Else
    //              c -         Throw an error
    //              d - Return the Block Directive
    static s_dBlock    stt_parseDirectiveBlock(std::string _name, std::ifstream &ifs,
            char &c)
    {
        s_dBlock   blockDirective;

        if (c != '{')
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        if (!ifs.get(c))
            throw (static_cast<int>(utl::err_BadDirectiveBlock));

        blockDirective.name = _name;

        std::string     name;

        while (c != '}')
        {
            if (stt_consumeNonToken(ifs, c) == EOF)
                throw (static_cast<int>(utl::err_BadDirectiveBlock));

            if (c == '}')
            {
                stt_get(ifs, c);
                break ;
            }

            if (c == ';' || c == '{')
                throw (static_cast<int>(utl::err_BadDirectiveBlock));

            if (stt_parseToken(ifs, c, name) == EOF)
                throw (static_cast<int>(utl::err_BadDirectiveBlock));

            map_dTable::iterator      it;

            if ((it = directiveTable.find(name)) == directiveTable.end())
                throw (static_cast<int>(utl::err_BadDirectiveBlock));

            if (it->second & e_dir_block)
            {
                if (stt_consumeNonToken(ifs, c) == EOF)
                    throw (static_cast<int>(utl::err_BadDirectiveBlock));

                if (name == "location")
                {
                    if (stt_parseToken(ifs, c, name) == EOF)
                        throw (static_cast<int>(utl::err_BadDirectiveBlock));

                    if (stt_consumeNonToken(ifs, c) == EOF)
                        throw (static_cast<int>(utl::err_BadDirectiveBlock));
                }

                s_dBlock   tmp = stt_parseDirectiveBlock(name, ifs, c);

                blockDirective.dBlock.insert(std::make_pair(it->second, tmp));
            }
            else if(it->second & e_dir_simple)
            {
                utl::sVector    tmp = stt_parseDirectiveSimple(ifs, c);

                blockDirective.dSimple.insert(std::make_pair(it->second, tmp));
            }
            else
                throw (static_cast<int>(utl::err_BadDirectiveBlock));
        }

        stt_get(ifs, c);

        return  (blockDirective);
    }

    // @brief:      parses multiple server Block Directives and saves it
    //                  int a vector
    //
    // @PARAM:      ifstream - config file stream; c - holds next char;
    //
    // @return:     vector<s_dBlock>, an array of multiple s_dBlocks that
    //                  holds the server blocks which will provide the server
    //                  class the necessary information for its instantiations
    //
    // @details:    1 - While theres characters in ifstream
    //              2 -     get next token
    //              3 -     If next token is "server"
    //              4 -         parse server as a Block Directive
    //              5 -     Else
    //              6 -         throw an error
    //              7 - Return Block Directives vector
    static s_dBlock stt_parseConfigSyntax(std::ifstream &ifs)
    {
        char            c;
        std::string     arg;
        s_dBlock   config;

        if (!ifs.get(c))
            throw (static_cast<int>(utl::err_BadConfig));

        if (stt_consumeNonToken(ifs, c) == EOF)
            throw (static_cast<int>(utl::err_BadConfig));

        if (stt_parseToken(ifs, c, arg) == EOF)
            throw (static_cast<int>(utl::err_BadConfig));

        if (arg != "http")
            throw (static_cast<int>(utl::err_BadConfig));

        if (stt_consumeNonToken(ifs, c) == EOF)
            return (config);

        config = stt_parseDirectiveBlock(arg, ifs, c);

        while (1)
        {
            if (!ifs.get(c))
                return (config);

            if (stt_consumeComments(ifs, c) == EOF)
                return (config);
            else if (stt_consumeNewLines(ifs, c) == EOF)
                return (config);
            else
                throw (static_cast<int>(utl::err_BadConfig));
        }

    }

    static int stt_portGet(const utl::sVector &listenBlock)
    {
        if (listenBlock.size() != 1)
            throw(static_cast<int>(utl::err_BadPort));

        std::string     digits;

        errno = 0;
        char    *end = NULL;
        long    portNo = strtol(listenBlock[0].c_str(), &end, 10);

        if (errno == ERANGE)
        {
            return (-1);
        }
        if (end == digits.c_str() || *end != '\0')
        {
            throw (static_cast<int>(utl::err_BadPortRange));
        }
        if (portNo < 1025 || portNo > 65535)
        {
            throw (static_cast<int>(utl::err_BadPortRange));
        }

        return (static_cast<int>(portNo));
    }

    // TODO:
    //      1- Verify ipVector validity
    //      2- Maybe use already the ip converter function already
    static std::string stt_ipGet(const utl::sVector &hostBlock)
    {
        if (hostBlock.size() != 1)
            throw(static_cast<int>(utl::err_BadIp));

        return (hostBlock[0]);
    }

    // TODO:
    //      1- Parse numbers then one letter
    //      2- Parse only the letters k, m, g and only one letter
    //      3- Multiple number by letter multiplier representation
    //      4- Make an enum
    static long    stt_clientMaxBodySizeGet(
            const utl::sVector &clientMaxBodySizeVector)
    {
        if (clientMaxBodySizeVector.size() != 1)
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));

        errno = 0;
        char        *ptr = NULL;
        const char  *clientMaxBodySizeStr = clientMaxBodySizeVector[0].c_str();
        size_t      clientMaxBodySizeStrLen = strlen(clientMaxBodySizeStr);
        size_t      size = std::strtol(clientMaxBodySizeStr, &ptr, 10);

        if (errno == ERANGE)
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));

        if  (*clientMaxBodySizeStr == '-')
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));

        if (ptr == clientMaxBodySizeStr)
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));

        if (strlen(ptr) > 1)
            throw(static_cast<int>(utl::err_BadClientMaxBodySize));

        switch (std::toupper(*(ptr++)))
        {
            case 'K':
                if (clientMaxBodySizeStrLen > 14)
                    throw(static_cast<int>(utl::err_BadClientMaxBodySize));
                size *= 1024;
                break ;
            case 'M':
                if (clientMaxBodySizeStrLen > 10)
                    throw(static_cast<int>(utl::err_BadClientMaxBodySize));
                size *= 1024 * 1024;
                break ;
            case 'G':
                if (clientMaxBodySizeStrLen > 6)
                    throw(static_cast<int>(utl::err_BadClientMaxBodySize));
                size *= 1024UL * 1024 * 1024;
                break ;
            case '\0':
                if (clientMaxBodySizeStrLen > 18)
                    throw(static_cast<int>(utl::err_BadClientMaxBodySize));
                break ;
            default:
                throw(static_cast<int>(utl::err_BadClientMaxBodySize));
        }

        return (size);
    }

    // @brief: Cache the error page response
    static int  stt_errorPageResponseBuild(std::string &errorPageResponse,
                                           const std::string &errorPagePath,
                                           const size_t code)
    {
        std::ostringstream      statusCode;
        std::string             reasonPhrase;
        utl::lsMap::iterator    it;
        std::string             extraLine;
        std::ostringstream      bodyLength;
        std::string             body;

        errorPageResponse = errorPageResponseTemplate;
        statusCode << static_cast<int>(code);
        reasonPhrase = reasonPhraseTable[static_cast<long>(code)];
        if ((it = extraLineTable.find(static_cast<int>(code)))
                != extraLineTable.end())
            extraLine = it->second;
        if (utl::fileToString(errorPagePath, body) == -1)
            return (-1);
        bodyLength << static_cast<int>(body.size());


        errorPageResponse.replace(errorPageResponse.find("<STATUS_CODE>"),
                         STATUS_CODE_LEN,
                         statusCode.str());
        errorPageResponse.replace(errorPageResponse.find("<REASON_PHRASE>"),
                         REASON_PHRASE_LEN,
                         reasonPhrase);
        errorPageResponse.replace(errorPageResponse.find("<EXTRA_LINE>"),
                         EXTRA_LINE_LEN,
                         extraLine);
        errorPageResponse.replace(errorPageResponse.find("<BODY_LENGTH>"),
                         BODY_LENGTH_LEN,
                         bodyLength.str());
        errorPageResponse.replace(errorPageResponse.find("<BODY>"),
                         BODY_LEN,
                         body);

        return (0);
    }

    // TODO:
    //      1- Iterate error pages files in the directory
    //      2- Check if all mandatory exist(by name)
    //      3- Check if all are html
    //      4- Check if the html is runnable(???????)
    //      5- Save the page on map_response
    static utl::lsMap stt_errorPagesGet(const utl::sVector &errorPagesBlock)
    {
        if (errorPagesBlock.size() != 1)
            throw(static_cast<int>(utl::err_BadErrorPages));

        DIR *errorPagesDir = opendir(errorPagesBlock[0].c_str());

        if (!errorPagesDir)
            throw(static_cast<int>(utl::err_BadErrorPagesDir));

        struct dirent   *entry;
        size_t          errorPagesCount = 0;
        utl::lsMap      errorPagesResponses;

        while ((entry = readdir(errorPagesDir)) != NULL)
        {
            std::string file = entry->d_name;

            if (file == "." || file == "..")
                continue ;

            if (!std::isdigit(file[0]))
                continue ;

            char    *ptr = NULL;
            long    errorCode = std::strtol(file.c_str(), &ptr, 10);

            if (ptr == file)
                continue ;

            if (std::strcmp(ptr, ".html") != 0)
                continue ;

            if ((errorCode >= 400 && errorCode <= 418)
                    || (errorCode >= 421 && errorCode <= 426)
                    || (errorCode == 428) || (errorCode == 429)
                    || (errorCode == 431) || (errorCode == 451)
                    || (errorCode >= 500 && errorCode <= 508)
                    || (errorCode == 510) || (errorCode == 511))
            {
                std::string errorPagePath = errorPagesBlock[0] + file;
                std::string errorPageResponse;

                if (stt_errorPageResponseBuild(errorPageResponse,
                            errorPagePath, errorCode) == -1)
                    continue;
                errorPagesResponses.insert(
                        std::make_pair(errorCode, errorPageResponse));
                errorPagesCount++;
            }
        }

        if (errorPagesCount != ERROR_PAGES_SIZE)
            throw (static_cast<int>(utl::err_BadErrorPagesAmount));

        closedir(errorPagesDir);

        return (errorPagesResponses);
    }

    // @details:    1 - Verify if path exists
    //              2 - cache response
    static std::string  stt_rootGet()
    {
        std::string root;

        DIR *errorPagesDir = opendir(errorPagesBlock[0].c_str());

        return (root);
    }

    // @details:    1 - Verify if args are valid
    //              2 - Set each method o true or false
    static void stt_methodsGet()
    {
    }

    // @details:    1 - Verify if arg is valid
    //              2 - Rerturn true or false
    static bool stt_autoindexGet()
    {
    }

    // @details:    1 - If path exists
    //              2 -     Return cached response
    //              3 - If AutoIndex is true
    //              4 -     Create html
    //              5 -     Return cached response
    //              6 - Throw exception
    static std::string  stt_indexGet()
    {
        std::string index;


        return (index);
    }

    // @details:    1 - Check if path is valid
    //              2 - Return path
    static std::string  stt_uploadPathGet()
    {
        std::string uploadPath;


        return (uploadPath);
    }

    // @details: TO BE DEFINED
    /*static void stt_cgiGet()
    {
    }*/

    static mmap_location    stt_locationGet(mmap_dBlock::iterator &itLocation,
                                         mmap_dBlock::iterator &locationEnd)
    {
        mmap_location           locations;

        for ( ; itLocation != locationEnd; itLocation++)
        {
            s_dBlock                dBlock = itLocation->second;
            mmap_dSimple            dSimple = dBlock.dSimple;
            mmap_dSimple::iterator  dSimpleEnd = dSimple.end();
            mmap_dSimple::iterator  itSimple;
            s_location              location;

            if (dSimple.size() != 5)
                throw(static_cast<int>(utl::err_BadDirectiveSimple));

            if (dSimple.count(e_root) != 1)
                throw(static_cast<int>(utl::err_MissingRoot));
            itSimple = dSimple.lower_bound(e_methods);
            location.root = stt_rootGet();

            /*if (dSimple.count(e_methods) != 1)
                throw(static_cast<int>(utl::err_MissingMethods));
            itSimple = dSimple.lower_bound(e_methods);
            location.methods = stt_methodsGet();

            if (dSimple.count(e_autoindex) != 1)
                throw(static_cast<int>(utl::err_MissingAutoIndex));
            itSimple = dSimple.lower_bound(e_methods);
            location.autoIndex = stt_autoIndexGet();

            if (dSimple.count(e_index) != 1)
                throw(static_cast<int>(utl::err_MissingAutoIndex));
            itSimple = dSimple.lower_bound(e_methods);
            location.index = stt_indexGet();

            if (dSimple.count(e_upload_path) != 1)
                throw(static_cast<int>(utl::err_MissingAutoIndex));
            itSimple = dSimple.lower_bound(e_methods);
            location.uploadPath = stt_uploadPathGet();*/
        }

        return (locations);
    }

    // @brief:  parse configuration semantic wise into the final configuration
    //          structure that will be then passed to server object
    static s_config   stt_parseConfigSemantics(const s_dBlock &httpBlock)
    {
        // Simple Directives
        s_config                   config;
        mmap_dSimple               dSimple = httpBlock.dSimple;
        mmap_dSimple::iterator     itSimple;
        mmap_dSimple::iterator     dSimpleEnd = dSimple.end();

        if (dSimple.size() != 4)
            throw(static_cast<int>(utl::err_BadDirectiveSimple));

        if (dSimple.count(e_listen) != 1)
            throw(static_cast<int>(utl::err_MissingPort));
        itSimple = dSimple.lower_bound(e_listen);
        config.port = stt_portGet(itSimple->second);

        if (dSimple.count(e_host) != 1)
            throw(static_cast<int>(utl::err_MissingIp));
        itSimple = dSimple.lower_bound(e_host);
        config.ip = stt_ipGet(itSimple->second);

        if (dSimple.count(e_client_max_body_size) != 1)
            throw(static_cast<int>(utl::err_MissingClientMaxBodySize));
        itSimple = dSimple.lower_bound(e_client_max_body_size);
        config.clientMaxBodySize = stt_clientMaxBodySizeGet(itSimple->second);

        if (dSimple.count(e_error_pages) != 1)
            throw(static_cast<int>(utl::err_MissingErrorPages));
        itSimple = dSimple.lower_bound(e_error_pages);
        config.errorPages = stt_errorPagesGet(itSimple->second);

        mmap_dBlock             dBlock = httpBlock.dBlock;
        mmap_dBlock::iterator   itBlock;
        mmap_dBlock::iterator   dBlockEnd = dBlock.end();
        mmap_dBlock::iterator   locationEnd = dBlock.upper_bound(e_location);

        if (dBlock.count(e_location) != dBlock.size())
            throw(static_cast<int>(utl::err_MissingLocation));
        if ((itBlock = dBlock.lower_bound(e_location)) == dBlockEnd)
            throw(static_cast<int>(utl::err_MissingLocation));
        config.location = stt_locationGet(itBlock, locationEnd);

        return (config);
    }

    // @brief:  opens the configuration file and calls it's parser function
    //
    // @PARAM:  filename - webserver configuration file path
    //
    // @return: vector<s_dBlock>, an array of multiple s_dBlocks that
    //              holds the server blocks which will provide the server
    //              class the necessary information for its instantiations
    std::vector<s_config>  fetch_config(char *filename)
    {
        configuration_file_init();

        std::ifstream   ifs(filename);
        if (!ifs)
        {
            std::cerr << "ERROR: Cannot open config file: "
                << filename << std::endl;
            throw (static_cast<int>(utl::err_FileOpening));
        }

        mmap_dBlock::iterator   it;
        std::vector<s_config>   configs;
        std::string             digits;
        s_dBlock                httpBlock = stt_parseConfigSyntax(ifs);

        if ((it = httpBlock.dBlock.lower_bound(e_server)) == httpBlock.dBlock.end())
            throw (static_cast<int>(utl::err_BadHttpBlock));

        for ( ; it != httpBlock.dBlock.end(); it++)
            configs.push_back(stt_parseConfigSemantics(it->second));
       
        return (configs);
    }
}
