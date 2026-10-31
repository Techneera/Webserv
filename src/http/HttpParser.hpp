
#ifndef HTTP_PARSER_HPP
# define HTTP_PARSER_HPP

# include <string>
# include <stdexcept>
# include "HttpRequest.hpp"

/**
 * @enum ParsingStage_e
 * @brief Represents the current stage of HTTP request parsing.
 */
enum ParsingStage_e
{
    STATE_REQUEST_LINE_e,
    STATE_FIELD_LINE_e,
    STATE_MESSAGE_BODY_e
};

/**
 * @enum ParsingResult_e
 * @brief Represents the outcome of a parsing operation.
 */
enum ParsingResult_e
{
    STATE_DONE_e,
    STATE_INCOMPLETE_e,
    STATE_ERROR_e
};

/**
 * @class HttpParser
 * @brief Parses HTTP requests and extracts their components.
 *
 * Processes incoming HTTP request data incrementally,
 * maintaining parsing progress between feed() calls.
 *
 * HttpParser owns the HttpRequest instance and controls
 * its lifetime. External access is provided through
 * a constant reference once parsing is complete.
 */
class HttpParser
{
    private:
        HttpRequest*    request;
        ParsingResult_e result;
        ParsingStage_e  stage;
        std::string     data;

        // Disable copying to prevent shallow copying.
        HttpParser(const HttpParser& other);
        HttpParser& operator=(const HttpParser& other);

        /**
         * @brief Coordinates the HTTP parsing process.
         */
        void parse();

        /**
         * @brief Parses the HTTP request line.
         */
        void parseRequestLine();

        /**
         * @brief Parses HTTP header fields.
         */
        void parseFieldLine();

        /**
         * @brief Parses the HTTP message body.
         */
        void parseMessageBody();

    public:
        /**
         * @brief Constructs an HTTP parser.
         */
        HttpParser();

        /**
         * @brief Destroys the parser and releases its resources.
         */
        ~HttpParser();

        /**
         * @brief Feeds incoming data into the parser.
         * @param input Incoming HTTP request data.
         * @return Current parsing result.
         */
        ParsingResult_e feed(const char* input);

        /**
         * @brief Retrieves the completed HTTP request.
         * @return Constant reference to the parsed request.
         *
         * @throws std::logic_error if parsing is incomplete
         *         or has failed.
         */
        const HttpRequest& getRequest() const;
};

#endif
