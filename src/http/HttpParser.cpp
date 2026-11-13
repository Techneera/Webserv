#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include <iostream>
#include <sstream>
#include <map>
#include <utility>
#include <vector>

HttpParser::HttpParser(
) : request(new HttpRequest_t),
    result(STATE_INCOMPLETE_e),
    stage(STATE_REQUEST_LINE_e) {}

HttpParser::~HttpParser()
{
    delete request;
}

void HttpParser::parse()
{
    switch (this->stage)
    {
        case STATE_REQUEST_LINE_e:
            parseRequestLine();
            break;

        case STATE_FIELD_LINE_e:
            //parseFieldLine();
            break;

        case STATE_MESSAGE_BODY_e:
            //parseMessageBody();
            break;

        default:
            return ;
    }
}

void HttpParser::parseRequestLine()
{
    size_t found = this->buffer.find("\r\n");

    if (found == std::string::npos)
        return ;

    std::istringstream  requestLineStream(this->buffer.substr(0, found));
    std::string         method;
    std::string         target;
    std::string         version;

    this->buffer.erase(0, found + 2);
    requestLineStream >> method >> target >> version;

    if (method != "GET" &&
        method != "POST" &&
        method != "DELETE" &&
        version != "HTTP/1.1")
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    size_t foundQuestionMark = target.find("?");
    std::vector<std::string> params;
    if (foundQuestionMark != std::string::npos)
    {
        std::string query = target.substr(found + 1, target.size());
        size_t foundAmpersand = 0;
        while (query != "")
        {
            foundAmpersand = query.find("&");
            if (foundAmpersand != std::string::npos)
            {
                params.push_back(query.substr(0, foundAmpersand));
                query.erase(0, foundAmpersand + 1);
            }
            else
            {
                break;
            }
        }
    }

        size_t found = it->find("=");
        if (found != std::string::npos)
        {
            this->request->requestLine.requestTarget.query.insert(std::make_pair(it->substr(0, found), it->substr(found + 1)));
        }
        this->request->requestLine.requestTarget.query.insert(std::make_pair(*it, ""));
    }
    this->request->requestLine.method = method;
    this->request->requestLine.version = version;
}

ParsingResult_e HttpParser::feed(const char* input, size_t bytesRead)
{
    if (this->result != STATE_INCOMPLETE_e)
        return this->result;

    this->buffer.append(input, bytesRead);

    this->parse();

    return this->result;
}

const HttpRequest_t& HttpParser::getRequest() const
{
    if (this->result != STATE_DONE_e)
    {
        throw std::logic_error("Request not parsed.\n");
    }
    return (const HttpRequest_t&)this->request;
}
