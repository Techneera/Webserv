#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include <sstream>
#include <map>
#include <cctype>

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

    size_t i = 0;
    while (isToken(this->buffer[i]))
        ++i;
    if (this->buffer[i++] != ' ')
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    if (this->buffer[i++] != '/')
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    while (isPchar(this->buffer, i))
        ++i;

    if (this->buffer[i++] != ' ')
    {
        this->result = STATE_ERROR_e;
        return ;
    }
};

void HttpParser::parseQueryParam(std::string params)
{
    size_t foundEqualSign = params.find("=");
    if (foundEqualSign != std::string::npos)
    {
        this->request->requestLine.requestTarget.query.insert(
            std::make_pair(
                params.substr(0, foundEqualSign), params.substr(foundEqualSign + 1)
            )
        );
        return ;
    }
    this->request->requestLine.requestTarget.query.insert(
        std::make_pair(params, "")
    );
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
    //if (this->result != STATE_DONE_e)
    //{
    //    throw std::logic_error("Request not parsed.\n");
    //}
    return (const HttpRequest_t&)*this->request;
}

bool HttpParser::isToken(const char& c)
{
    return (
        c > 31 && c < 127 &&
        c != '"' && c != '(' &&
        c != ')' && c != ',' &&
        c != '/' && c != ':' &&
        c != ';' && c != '<' &&
        c != '=' && c != '>' &&
        c != '?' && c != '@' &&
        c != '[' && c != '\\' &&
        c != ']' && c != '{' && c != '}'
    );
}

bool HttpParser::isPchar(const std::string& str, size_t pos)
{
    return (
        std::isalnum(static_cast<unsigned char>(str[pos])) ||
        str[pos] == '-' || str[pos] == '.' ||
        str[pos] == '_' || str[pos] == '~' ||
        str[pos] == ':' || str[pos] == '@' ||
        isPctEncoded(str, pos) ||
        str[pos] == '!' || str[pos] == '$' ||
        str[pos] == '&' || str[pos] == '\'' ||
        str[pos] == '(' || str[pos] == ')' ||
        str[pos] == '*' || str[pos] == '+' ||
        str[pos] == ',' || str[pos] == ';' ||
        str[pos] == '='
    );
}

bool HttpParser::isPctEncoded(const std::string& str, size_t pos)
{
   return (
       str[pos] == '%' &&
       std::isxdigit(static_cast<unsigned char>(str[pos + 1])) &&
       std::isxdigit(static_cast<unsigned char>(str[pos + 2]))
   );
}
