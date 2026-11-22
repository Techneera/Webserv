#ifndef HTTP_PARSER_HPP
#define HTTP_PARSER_HPP

#include <cstddef>
#include <stdexcept>
#include <string>

#include "HttpRequest.hpp"

/**
 * @enum ParsingStage_e
 * @brief Identifies the section of the HTTP request currently being parsed.
 *
 * The parser advances through these stages as enough input becomes available.
 */
enum ParsingStage_e
{
    /** Parser has not started processing a request. */
    STATE_NOT_STARTED_e,

    /** Parser is processing the HTTP request line. */
    STATE_REQUEST_LINE_e,

    /** Parser is processing HTTP header fields. */
    STATE_FIELD_LINE_e,

    /** Parser is processing the HTTP message body. */
    STATE_MESSAGE_BODY_e
};

/**
 * @enum ParsingResult_e
 * @brief Represents the current result of parsing an HTTP request.
 */
enum ParsingResult_e
{
    /** The request has been parsed successfully. */
    STATE_DONE_e,

    /**
     * The request is not yet complete.
     *
     * More input may be supplied through HttpParser::feed().
     */
    STATE_INCOMPLETE_e,

    /** Invalid HTTP syntax was encountered or parsing was terminated early. */
    STATE_ERROR_e
};

/**
 * @class HttpParser
 * @brief Incrementally parses an HTTP request.
 *
 * HttpParser accepts request data through feed(), allowing an HTTP request to
 * be provided across multiple input chunks. The parser retains unprocessed
 * data and its parsing state between calls.
 *
 * Parsing continues until one of the following occurs:
 * - the request is successfully parsed and STATE_DONE_e is reached;
 * - invalid syntax is encountered and STATE_ERROR_e is reached;
 * - more input is required, leaving the parser in STATE_INCOMPLETE_e.
 *
 * finish() must be called when no more input will be supplied. If the request
 * is still incomplete at that point, the parsing result becomes STATE_ERROR_e.
 *
 * The parser owns the HttpRequest_t object it constructs. Once parsing has
 * completed, the parsed request can be accessed through getRequest().
 *
 * @note This class is non-copyable because it owns its HttpRequest_t instance.
 */
class HttpParser
{
private:
    /** HTTP request being populated during parsing. */
    HttpRequest_t* request;

    /** Current overall parsing result. */
    ParsingResult_e result;

    /** Current section of the HTTP request being parsed. */
    ParsingStage_e stage;

    /**
     * Parser-owned request data.
     *
     * @note Currently unused by the implementation.
     */
    std::string data;

    /**
     * Input accumulated through feed() and not yet discarded.
     *
     * Parsed data is removed from this buffer as parsing progresses.
     */
    std::string buffer;

    /** Start offset of the component currently being parsed. */
    size_t start;

    /** Current parsing cursor within buffer. */
    size_t end;

    /**
     * @brief Copy constructor disabled.
     *
     * Prevents shallow copying of the owned HttpRequest_t object.
     */
    HttpParser(const HttpParser& other);

    /**
     * @brief Copy assignment disabled.
     *
     * Prevents multiple parser instances from owning the same HttpRequest_t.
     */
    HttpParser& operator=(const HttpParser& other);

    /**
     * @brief Dispatches parsing according to the current parsing stage.
     *
     * The appropriate stage-specific parser is called using the current value
     * of stage.
     */
    void parse();

    /**
     * @brief Parses and validates the HTTP request line.
     *
     * Parses the request method, request target, optional query string, and
     * HTTP version from buffer.
     *
     * The expected request-line format is:
     *
     * @code
     * method SP request-target SP HTTP-version CRLF
     * @endcode
     *
     * If a complete CRLF-terminated request line has not yet been received,
     * the function returns without modifying the parsing result.
     *
     * On successful parsing, the corresponding fields in HttpRequest_t are
     * populated and result is set to STATE_DONE_e by the current
     * implementation.
     *
     * If invalid syntax is encountered, result is set to STATE_ERROR_e.
     */
    void parseRequestLine();

    /**
     * @brief Parses the HTTP header-field section.
     *
     * @note Header-field parsing is not currently implemented.
     */
    void parseFieldLine();

    /**
     * @brief Parses the HTTP message body.
     *
     * @note Message-body parsing is not currently implemented.
     */
    void parseMessageBody();

    /**
     * @brief Parses and validates the request method.
     *
     * Reads an HTTP token beginning at the current parsing position and stores
     * the method in the request line.
     *
     * The current implementation accepts GET, POST, and DELETE.
     *
     * @return STATE_INCOMPLETE_e when the method is valid.
     * @return STATE_ERROR_e when the method is invalid or unsupported.
     */
    ParsingResult_e parseRequestMethod();

    /**
     * @brief Parses and validates the HTTP version.
     *
     * Expects a version in the form "HTTP/x.y", where x and y are digits.
     * On success, the parsed version is stored in the request line.
     *
     * @return STATE_INCOMPLETE_e when the HTTP version is valid.
     * @return STATE_ERROR_e when the HTTP version is malformed.
     */
    ParsingResult_e parseHttpVersion();

    /**
     * @brief Parses the path component of the request target.
     *
     * The path must begin with '/' and may contain RFC 3986 pchar characters
     * and additional '/' separators.
     *
     * @return STATE_INCOMPLETE_e when the path is valid.
     * @return STATE_ERROR_e when the path does not begin with '/'.
     */
    ParsingResult_e parsePath();

    /**
     * @brief Parses the query component of the request target.
     *
     * Query parameters are separated using '&' and are delegated to
     * parseQueryParam() for insertion into the request query map.
     *
     * @return STATE_INCOMPLETE_e after parsing the query.
     */
    ParsingResult_e parseQuery();

    /**
     * @brief Parses the current query parameter.
     *
     * Parameters of the form "key=value" are inserted into the request query
     * map as a key/value pair. Parameters without '=' are treated as flags and
     * inserted with an empty value.
     */
    void parseQueryParam();

    /**
     * @brief Determines whether a character is valid in an HTTP token.
     *
     * @param c Character to validate.
     *
     * @return true if c is accepted as a token character.
     * @return false otherwise.
     */
    bool isToken(const char& c);

    /**
     * @brief Determines whether input at a given position begins a valid pchar.
     *
     * Accepts alphanumeric characters, RFC 3986 unreserved characters,
     * sub-delimiters, ':' and '@'. Percent-encoded sequences are validated
     * through isPctEncoded().
     *
     * @param str String containing the character or sequence to validate.
     * @param pos Position within str to inspect.
     *
     * @return true if a valid pchar begins at pos.
     * @return false otherwise.
     */
    bool isPchar(const std::string& str, size_t pos);

    /**
     * @brief Determines whether input at a position is percent-encoded.
     *
     * A valid percent-encoded sequence has the form "%HH", where each H is a
     * hexadecimal digit.
     *
     * @param str String containing the sequence to inspect.
     * @param pos Position expected to contain '%'.
     *
     * @return true if a valid percent-encoded sequence begins at pos.
     * @return false otherwise.
     */
    bool isPctEncoded(const std::string& str, size_t pos);

public:
    /**
     * @brief Constructs a new HTTP parser.
     *
     * Allocates an empty HttpRequest_t and initializes the parser in the
     * request-line stage with an incomplete result.
     */
    HttpParser();

    /**
     * @brief Destroys the parser.
     *
     * Releases the HttpRequest_t owned by this parser.
     */
    ~HttpParser();

    /**
     * @brief Supplies another chunk of request data to the parser.
     *
     * The supplied bytes are appended to the parser's internal buffer before
     * parsing continues from the current stage.
     *
     * If parsing has already completed or failed, the input is ignored and
     * the existing parsing result is returned.
     *
     * @param input Pointer to the incoming request data.
     * @param bytesRead Number of bytes from input to consume.
     *
     * @return STATE_INCOMPLETE_e if additional data is required.
     * @return STATE_DONE_e if parsing has completed successfully.
     * @return STATE_ERROR_e if invalid syntax is encountered.
     */
    ParsingResult_e feed(const char* input, size_t bytesRead);

    /**
     * @brief Retrieves the HTTP request currently owned by the parser.
     *
     * @return Constant reference to the parsed HttpRequest_t.
     *
     * @warning The current implementation does not enforce that parsing has
     * completed successfully before returning the request.
     */
    const HttpRequest_t& getRequest() const;

    /**
     * @brief Signals that no more request data will be supplied.
     *
     * If parsing is still incomplete when this function is called, the request
     * is considered malformed or truncated and the result becomes
     * STATE_ERROR_e.
     *
     * Calling finish() after parsing has already succeeded or failed leaves
     * the existing result unchanged.
     *
     * @return Final parsing result.
     */
    ParsingResult_e finish();
};

#endif
