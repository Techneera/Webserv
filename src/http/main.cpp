#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include <iostream>

void testFeed(const char* request, size_t bytesToRead)
{
    size_t start = 0;
    std::string requestStr(request);
    std::string chunk;
    HttpParser  parser;

    std::cout << "Test: " << request << std::endl;
    while ((chunk = requestStr.substr(requestStr.size() > start ? start : requestStr.size(), bytesToRead)) != "")
    {
        parser.feed(chunk.c_str(), bytesToRead);
        start += bytesToRead;
    }
}

int main()
{
    HttpParser parser;
    HttpRequest_t request;
    std::string requestLine = "GET /users/42?name=jcesar-s&age=24 HTTP/1.1\r\n";

    parser.feed(requestLine.c_str(), requestLine.size());
    request = parser.getRequest();
    std::cout << request.requestLine.requestTarget.path << std::endl;
    std::multimap<std::string, std::string>::iterator it = request.requestLine.requestTarget.query.begin();
    std::cout << it->first << ": " << it->second << std::endl;

    ++it;
    std::cout << it->first << ": " << it->second << std::endl;
    return (0);
}
