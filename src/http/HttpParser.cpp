#include "HttpParser.hpp"
#include "HttpRequest.hpp"
#include <map>
#include <cctype>
#include <cstring>

HttpParser::HttpParser(
) : request(new HttpRequest_t),
    result(STATE_INCOMPLETE_e),
    stage(STATE_REQUEST_LINE_e),
    start(0),
    end(0) {}

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

    if ((this->result = parseRequestMethod()) == STATE_ERROR_e)
        return ;

    if (this->buffer[this->end] != ' ')
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    this->start = ++this->end;

    if ((this->result = parsePath()) == STATE_ERROR_e)
        return ;

    if (this->buffer[this->end] != ' ' && this->buffer[this->end] != '?')
    {
        this->result = STATE_ERROR_e;
        return ;
    }


    if (this->buffer[this->end] == '?')
    {
        this->start = ++this->end;
        parseQuery();
    }

    if (this->buffer[this->end] != ' ')
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    this->start = ++this->end;

    if ((this->result = parseHttpVersion()) == STATE_ERROR_e)
        return ;

    if (this->buffer.size() >= 2 &&
        this->buffer.compare(this->end, 2, "\r\n") != 0)
    {
        this->result = STATE_ERROR_e;
        return ;
    }

    this->buffer.erase(0, found + 2);
    this->result = STATE_DONE_e;
};

void HttpParser::parseQueryParam()
{
    size_t foundEqualSign = this->buffer.find("=", this->start);
    if (foundEqualSign != std::string::npos)
    {
        size_t queryEnd = this->end - foundEqualSign - (this->buffer[this->end] == ' ');
        this->request->requestLine.requestTarget.query.insert(
            std::make_pair(
                this->buffer.substr(this->start, foundEqualSign - this->start),
                this->buffer.substr(foundEqualSign + 1, queryEnd)
            )
        );
        return ;
    }
    std::string flag = this->buffer.substr(this->start, this->end - this->start);
    if (!flag.empty())
        this->request->requestLine.requestTarget.query.insert(
            std::make_pair(flag, "")
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
        c > 32 && c < 127 &&
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

ParsingResult_e HttpParser::parseRequestMethod()
{
    std::string method;
    while (isToken(this->buffer[this->end]))
        this->end++;

    method = buffer.substr(0, this->end);
    if (method != "GET" &&
        method != "POST" &&
        method != "DELETE")
        return STATE_ERROR_e;
    this->start = end;
    this->request->requestLine.method = method;
    return STATE_INCOMPLETE_e;
}

ParsingResult_e HttpParser::parseHttpVersion()
{
    if (this->buffer.compare(this->start, 4, "HTTP") != 0)
        return STATE_ERROR_e;
    this->end += 4;
    if (!std::isdigit(this->buffer[++this->end]))
        return STATE_ERROR_e;
    if (this->buffer[++this->end] != '.')
        return STATE_ERROR_e;
    if (!std::isdigit(this->buffer[++this->end]))
        return STATE_ERROR_e;
    this->request->requestLine.version =
          this->buffer.substr(this->start, ++this->end - this->start);
    return STATE_INCOMPLETE_e;
}

ParsingResult_e HttpParser::parsePath()
{
    if (this->buffer[this->end++] != '/')
        return STATE_ERROR_e;
    while (isPchar(this->buffer, this->end) || this->buffer[this->end] == '/')
        ++this->end;

    this->request->requestLine.requestTarget.path = this->buffer.substr(this->start, this->end - this->start);
    return STATE_INCOMPLETE_e;
}

ParsingResult_e HttpParser::parseQuery()
{
    while (isPchar(this->buffer, this->end) || this->buffer[this->end] == '/'
                || this->buffer[this->end] == '?')
    {
        if (this->buffer[this->end] == '&')
        {
            --this->end;
            parseQueryParam();
            this->start = ++this->end + 1;
        }
        ++this->end;
    }
    parseQueryParam();
    return STATE_INCOMPLETE_e;
}

ParsingResult_e HttpParser::finish()
{
    if (this->result == STATE_INCOMPLETE_e)
        this->result = STATE_ERROR_e;

    return this->result;
}
